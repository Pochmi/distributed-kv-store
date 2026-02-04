#include "cluster_config.h"
#include <fstream>
#include <sstream>
#include <functional>
#include <iostream>
#include <cstdlib>
#include <algorithm>

ClusterConfig::ClusterConfig() {
    // 构造函数中尝试加载配置
    const char* config_env = std::getenv("KV_CLUSTER_CONFIG");
    if (config_env) {
        config_file_ = config_env;
        if (loadFromFile(config_file_)) {
            return;
        }
    }
    
    // 尝试默认配置文件
    if (loadFromFile("configs/cluster_3nodes.json")) {
        return;
    }
    
    // 如果都没有，使用默认配置
    initDefaultConfig();
}

ClusterConfig& ClusterConfig::getInstance() {
    static ClusterConfig instance;
    return instance;
}

bool ClusterConfig::loadFromFile(const std::string& config_file) {
    std::ifstream file(config_file);
    if (!file.is_open()) {
        std::cerr << "[Cluster] 无法打开配置文件: " << config_file << std::endl;
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    return loadFromJson(buffer.str());
}

bool ClusterConfig::loadFromJson(const std::string& json_str) {
    std::cout << "[Cluster] 加载集群配置..." << std::endl;
    
    // 清空现有节点
    nodes_.clear();
    
    // 简单解析JSON（为了毕业设计，这里简化处理）
    // 查找nodes数组
    size_t pos = json_str.find("\"nodes\"");
    if (pos == std::string::npos) {
        std::cerr << "[Cluster] 配置文件中未找到nodes字段" << std::endl;
        return false;
    }
    
    // 解析每个节点
    size_t start = json_str.find('[', pos);
    size_t end = json_str.find(']', start);
    
    if (start == std::string::npos || end == std::string::npos) {
        std::cerr << "[Cluster] 节点列表格式错误" << std::endl;
        return false;
    }
    
    std::string nodes_str = json_str.substr(start + 1, end - start - 1);
    
    // 简单解析：查找每个节点的port字段
    size_t node_start = 0;
    int node_count = 0;
    
    while ((node_start = nodes_str.find("\"port\"", node_start)) != std::string::npos) {
        size_t colon = nodes_str.find(':', node_start);
        size_t comma = nodes_str.find(',', colon);
        if (comma == std::string::npos) {
            comma = nodes_str.find('}', colon);
        }
        
        std::string port_str = nodes_str.substr(colon + 1, comma - colon - 1);
        port_str.erase(std::remove_if(port_str.begin(), port_str.end(), 
                      [](unsigned char c){ return std::isspace(c); }), port_str.end());
        
        int port = std::stoi(port_str);
        
        NodeInfo node;
        node.id = "server-" + std::to_string(node_count + 1);
        node.host = "127.0.0.1";
        node.port = port;
        node.role = "master";
        node.is_healthy = true;
        node.shard_id = node_count;
        
        nodes_.push_back(node);
        node_count++;
        node_start = comma;
    }
    
    if (nodes_.empty()) {
        std::cerr << "[Cluster] 未找到有效节点配置" << std::endl;
        return false;
    }
    
    config_loaded_ = true;
    
    std::cout << "[Cluster] 成功加载 " << nodes_.size() << " 个节点:" << std::endl;
    for (const auto& node : nodes_) {
        std::cout << "[Cluster]   " << node.id << " 在 " << node.address() 
                  << " (分片: " << node.shard_id << ")" << std::endl;
    }
    
    return true;
}

void ClusterConfig::initDefaultConfig() {
    // 默认的3节点配置 - 使用旧的初始化方式
    nodes_.clear();  // 清空向量
    
    // 逐个添加节点
    NodeInfo node1;
    node1.id = "server-1";
    node1.host = "127.0.0.1";
    node1.port = 6381;
    node1.role = "master";
    node1.is_healthy = true;
    node1.shard_id = 0;
    nodes_.push_back(node1);
    
    NodeInfo node2;
    node2.id = "server-2";
    node2.host = "127.0.0.1";
    node2.port = 6382;
    node2.role = "master";
    node2.is_healthy = true;
    node2.shard_id = 1;
    nodes_.push_back(node2);
    
    NodeInfo node3;
    node3.id = "server-3";
    node3.host = "127.0.0.1";
    node3.port = 6383;
    node3.role = "master";
    node3.is_healthy = true;
    node3.shard_id = 2;
    nodes_.push_back(node3);
    
    config_loaded_ = true;
    
    std::cout << "[Cluster] 使用默认3节点配置:" << std::endl;
    for (const auto& node : nodes_) {
        std::cout << "[Cluster]   " << node.id << " 在 " << node.address() 
                  << " (分片: " << node.shard_id << ")" << std::endl;
    }
}

NodeInfo ClusterConfig::getNodeByKey(const std::string& key) {
    if (!config_loaded_) {
        initDefaultConfig();
    }
    
    if (nodes_.empty()) {
        throw std::runtime_error("集群中没有可用节点");
    }
    
    // 哈希取模分片
    std::hash<std::string> hash_fn;
    size_t hash_value = hash_fn(key);
    size_t node_index = hash_value % nodes_.size();
    
    NodeInfo target_node = nodes_[node_index];
    std::cout << "[Cluster] 键 '" << key << "' -> " 
              << target_node.id << " (" << target_node.address() 
              << ", 分片: " << target_node.shard_id << ")" << std::endl;
    
    return target_node;
}

std::vector<NodeInfo> ClusterConfig::getAllNodes() const {
    return nodes_;
}

size_t ClusterConfig::getNodeCount() const {
    return nodes_.size();
}

void ClusterConfig::markNodeUnhealthy(const std::string& node_id) {
    for (auto& node : nodes_) {
        if (node.id == node_id) {
            node.is_healthy = false;
            std::cout << "[Cluster] 标记节点 " << node_id << " 为不健康" << std::endl;
            break;
        }
    }
}

void ClusterConfig::markNodeHealthy(const std::string& node_id) {
    for (auto& node : nodes_) {
        if (node.id == node_id) {
            node.is_healthy = true;
            std::cout << "[Cluster] 标记节点 " << node_id << " 为健康" << std::endl;
            break;
        }
    }
}
