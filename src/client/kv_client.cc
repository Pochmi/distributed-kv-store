#include "kv_client.h"
#include <iostream>
#include <sstream>
#include <vector>

KVClient::KVClient() {
    router_.reset(new Router());  // 使用 new 而不是 make_unique
    std::cout << "[KVClient] 客户端初始化完成" << std::endl;
}

KVClient::~KVClient() {
    if (current_connection_ && current_connection_->isConnected()) {
        current_connection_->disconnect();
    }
}

bool KVClient::connectToNode(const NodeInfo& node) {
    if (current_connection_ && current_connection_->isConnected()) {
        current_connection_->disconnect();
    }
    
    current_connection_.reset(new Connection(node.host, node.port));
    return current_connection_->connect();
}

std::string KVClient::executeCommand(const std::string& command) {
    if (!current_connection_ || !current_connection_->isConnected()) {
        throw std::runtime_error("未连接到服务器");
    }
    
    if (!current_connection_->send(command + "\n")) {
        throw std::runtime_error("发送命令失败");
    }
    
    std::string response = current_connection_->receive();
    return response;
}

std::string KVClient::executeWithRetry(const std::string& command, const std::string& key, int max_retries) {
    for (int attempt = 1; attempt <= max_retries; attempt++) {
        try {
            // 获取目标节点
            NodeInfo target_node = router_->route(key);
            
            // 连接到节点
            if (!connectToNode(target_node)) {
                std::cerr << "[KVClient] 第 " << attempt << " 次尝试: 连接失败" << std::endl;
                
                // 标记节点为不健康
                router_->markNodeUnhealthy(target_node.id);
                
                // 尝试其他节点
                if (attempt < max_retries) {
                    continue;
                }
                return "ERROR Connection failed";
            }
            
            // 执行命令
            std::string response = executeCommand(command);
            std::cout << "[KVClient] 服务器响应: " << response << std::endl;
            return response;
            
        } catch (const std::exception& e) {
            std::cerr << "[KVClient] 第 " << attempt << " 次尝试失败: " << e.what() << std::endl;
            
            if (attempt < max_retries) {
                std::cout << "[KVClient] 正在重试..." << std::endl;
            }
        }
    }
    
    return "ERROR Max retries exceeded";
}

bool KVClient::put(const std::string& key, const std::string& value) {
    std::string command = "PUT " + key + " " + value;
    std::string response = executeWithRetry(command, key);
    
    if (response.find("OK") != std::string::npos || 
        response.find("SUCCESS") != std::string::npos) {
        std::cout << "✅ SET 成功: " << key << " = " << value << std::endl;
        return true;
    } else {
        std::cout << "❌ SET 失败" << std::endl;
        return false;
    }
}

std::string KVClient::get(const std::string& key) {
    std::string command = "GET " + key;
    std::string response = executeWithRetry(command, key);
    
    if (response.find("ERROR") != std::string::npos) {
        std::cout << "键 '" << key << "' 不存在" << std::endl;
        return "";
    }
    
    // 解析响应，提取值
    size_t space_pos = response.find(' ');
    if (space_pos != std::string::npos) {
        return response.substr(space_pos + 1);
    }
    
    return response;
}

bool KVClient::del(const std::string& key) {
    std::string command = "DEL " + key;
    std::string response = executeWithRetry(command, key);
    
    if (response.find("OK") != std::string::npos || 
        response.find("SUCCESS") != std::string::npos) {
        std::cout << "✅ DELETE 成功: " << key << std::endl;
        return true;
    } else {
        std::cout << "❌ DELETE 失败" << std::endl;
        return false;
    }
}

bool KVClient::batchPut(const std::vector<std::pair<std::string, std::string>>& kvs) {
    bool all_success = true;
    
    for (const auto& kv : kvs) {
        if (!put(kv.first, kv.second)) {
            all_success = false;
        }
    }
    
    return all_success;
}

bool KVClient::ping() {
    try {
        // 尝试连接到任意一个节点
        auto nodes = router_->getAllNodes();
        if (nodes.empty()) {
            return false;
        }
        
        if (connectToNode(nodes[0])) {
            std::string response = executeCommand("PING");
            return response.find("PONG") != std::string::npos || 
                   response.find("OK") != std::string::npos;
        }
    } catch (...) {
        // 忽略异常
    }
    
    return false;
}
