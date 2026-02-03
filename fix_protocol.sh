#!/bin/bash

echo "=== 修复客户端协议 ==="
echo "将PUT改为SET，以匹配服务器协议"

# 创建修复后的kv_client.cc
cat > src/client/kv_client.cc.fixed << 'FIXED'
#include "kv_client.h"
#include "../common/protocol.h"
#include "../network/connection.h"
#include <thread>
#include <chrono>

KVClient::KVClient(const std::string& config_file) {
    router_ = std::make_shared<Router>();
    
    if (!config_file.empty()) {
        if (!ClusterConfig::getInstance().loadConfig(config_file)) {
            LOG_ERROR("Failed to load cluster config");
        }
    }
}

bool KVClient::put(const std::string& key, const std::string& value, int timeout_ms) {
    // 1. 路由到正确的节点
    NodeInfo target_node;
    try {
        target_node = router_->route(key);
    } catch (const std::exception& e) {
        LOG_ERROR("Routing failed: {}", e.what());
        return false;
    }
    
    // 2. 序列化请求 - 注意：服务器使用SET，不是PUT
    std::string request = serializeRequest("SET", key, value);
    
    // 3. 发送请求
    std::string response_str;
    bool success = sendRequest(target_node, request, response_str, timeout_ms);
    
    if (success) {
        // 简化处理：检查响应是否包含OK或Success
        if (response_str.find("OK") != std::string::npos || 
            response_str.find("Success") != std::string::npos) {
            return true;
        }
        return false;
    }
    
    return false;
}

std::string KVClient::get(const std::string& key, int timeout_ms) {
    NodeInfo target_node;
    try {
        target_node = router_->route(key);
    } catch (const std::exception& e) {
        LOG_ERROR("Routing failed: {}", e.what());
        return "";
    }
    
    std::string request = serializeRequest("GET", key);
    std::string response_str;
    
    if (sendRequest(target_node, request, response_str, timeout_ms)) {
        // 服务器返回格式：STATUS [MESSAGE]\n
        // 我们需要提取实际的数据
        // 简化：直接返回响应，稍后解析
        return response_str;
    }
    
    return "";
}

bool KVClient::del(const std::string& key, int timeout_ms) {
    NodeInfo target_node;
    try {
        target_node = router_->route(key);
    } catch (const std::exception& e) {
        LOG_ERROR("Routing failed: {}", e.what());
        return false;
    }
    
    std::string request = serializeRequest("DEL", key);
    std::string response_str;
    
    if (sendRequest(target_node, request, response_str, timeout_ms)) {
        if (response_str.find("OK") != std::string::npos || 
            response_str.find("Success") != std::string::npos) {
            return true;
        }
        return false;
    }
    
    return false;
}

bool KVClient::sendRequest(const NodeInfo& node, 
                          const std::string& request, 
                          std::string& response,
                          int timeout_ms) {
    // 简单的TCP连接实现
    // 实际项目中应该使用连接池
    try {
        Connection connection;
        if (!connection.connect(node.host, node.port, timeout_ms)) {
            LOG_ERROR("Failed to connect to {}:{}", node.host, node.port);
            return false;
        }
        
        // 发送请求
        if (!connection.send(request)) {
            LOG_ERROR("Failed to send request to {}", node.id);
            return false;
        }
        
        // 接收响应
        if (!connection.receive(response, timeout_ms)) {
            LOG_ERROR("Failed to receive response from {}", node.id);
            return false;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Network error to {}: {}", node.id, e.what());
        return false;
    }
}

std::string KVClient::serializeRequest(const std::string& cmd, 
                                      const std::string& key, 
                                      const std::string& value) {
    // 根据protocol.h的协议：COMMAND [ARG1] [ARG2] ...\n
    if (cmd == "SET") {
        return "SET " + key + " " + value + "\n";
    } else if (cmd == "GET") {
        return "GET " + key + "\n";
    } else if (cmd == "DEL") {
        return "DEL " + key + "\n";
    }
    
    throw std::invalid_argument("Unknown command: " + cmd);
}

bool KVClient::updateClusterConfig(const std::string& config_file) {
    return ClusterConfig::getInstance().loadConfig(config_file);
}

std::vector<std::string> KVClient::getClusterStatus() {
    auto config = ClusterConfig::getInstance();
    std::vector<std::string> status;
    
    // 这里简化实现，实际应该从每个节点获取状态
    auto masters = config.getAvailableMasters();
    for (const auto& node : masters) {
        std::string s = node.id + " (" + node.address() + ") - " + 
                       node.role + " - " + 
                       (node.is_available ? "UP" : "DOWN");
        status.push_back(s);
    }
    
    return status;
}
FIXED

echo "修复完成！新的文件保存为: src/client/kv_client.cc.fixed"
echo ""
echo "下一步："
echo "1. 备份原始文件: cp src/client/kv_client.cc src/client/kv_client.cc.orig"
echo "2. 使用修复后的文件: cp src/client/kv_client.cc.fixed src/client/kv_client.cc"
echo "3. 重新编译: cd build && make kv_client"
