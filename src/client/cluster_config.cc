#include "cluster_config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <functional>
#include <algorithm>

ClusterConfig::ClusterConfig() : config_loaded_(false) {
    // 构造函数中初始化默认3节点
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
        initDefaultConfig();
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json_str = buffer.str();
    file.close();
    
    if (!loadFromJson(json_str)) {
        std::cerr << "[Cluster] 配置文件解析失败，使用默认配置" << std::endl;
        initDefaultConfig();
        return false;
    }
    
    config_file_ = config_file;
    config_loaded_ = true;
    
    std::cout << "[Cluster] 成功加载 " << nodes_.size() << " 个节点:" << std::endl;
    for (const auto& node : nodes_) {
        std::cout << "[Cluster]   " << node.id << " 在 " << node.address() << std::endl;
    }
    return true;
}

bool ClusterConfig::loadFromJson(const std::string& json_str) {
    std::lock_guard<std::mutex> lock(mutex_);
    nodes_.clear();
    
    size_t nodes_pos = json_str.find("\"nodes\"");
    if (nodes_pos == std::string::npos) return false;
    
    size_t array_start = json_str.find('[', nodes_pos);
    size_t array_end = json_str.find(']', array_start);
    if (array_start == std::string::npos || array_end == std::string::npos) return false;
    
    std::string nodes_str = json_str.substr(array_start + 1, array_end - array_start - 1);
    
    size_t pos = 0;
    while (true) {
        size_t obj_start = nodes_str.find('{', pos);
        if (obj_start == std::string::npos) break;
        
        size_t obj_end = nodes_str.find('}', obj_start);
        if (obj_end == std::string::npos) break;
        
        std::string obj = nodes_str.substr(obj_start, obj_end - obj_start + 1);
        
        auto extract = [&](const std::string& field) -> std::string {
            size_t fpos = obj.find("\"" + field + "\"");
            if (fpos == std::string::npos) return "";
            size_t colon = obj.find(':', fpos);
            size_t q1 = obj.find('"', colon);
            size_t q2 = obj.find('"', q1 + 1);
            if (q1 == std::string::npos || q2 == std::string::npos) return "";
            return obj.substr(q1 + 1, q2 - q1 - 1);
        };
        
        NodeInfo node;
        node.id = extract("id");
        node.host = extract("host");
        std::string port_str = extract("port");
        if (!port_str.empty()) node.port = std::stoi(port_str);
        node.role = extract("role");
        if (node.role.empty()) node.role = "master";
        
        if (!node.id.empty() && !node.host.empty() && node.port > 0) {
            nodes_.push_back(node);
        }
        
        pos = obj_end + 1;
    }
    
    return !nodes_.empty();
}

void ClusterConfig::initDefaultConfig() {
    std::lock_guard<std::mutex> lock(mutex_);
    nodes_.clear();
    
    nodes_.push_back(NodeInfo("server-1", "127.0.0.1", 6381, "master", true, 0));
    nodes_.push_back(NodeInfo("server-2", "127.0.0.1", 6382, "master", true, 1));
    nodes_.push_back(NodeInfo("server-3", "127.0.0.1", 6383, "master", true, 2));
    
    std::cout << "[Cluster] 使用默认3节点配置:" << std::endl;
    for (const auto& node : nodes_) {
        std::cout << "[Cluster]   " << node.id << " 在 " << node.address() << std::endl;
    }
}

uint32_t ClusterConfig::hash(const std::string& key) const {
    return std::hash<std::string>{}(key);
}

NodeInfo ClusterConfig::getNodeByKey(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (nodes_.empty()) initDefaultConfig();
    
    std::vector<NodeInfo> healthy_masters;
    for (const auto& node : nodes_) {
        if (node.role == "master" && node.is_healthy) {
            healthy_masters.push_back(node);
        }
    }
    
    if (healthy_masters.empty()) return nodes_[0];
    
    uint32_t hash_value = hash(key);
    size_t index = hash_value % healthy_masters.size();
    
    NodeInfo result = healthy_masters[index];
    std::cout << "[Cluster] 键 '" << key << "' -> " << result.id 
              << " (" << result.address() << ")" << std::endl;
    return result;
}

std::vector<NodeInfo> ClusterConfig::getAvailableMasters() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 如果 nodes_ 为空，初始化默认配置
    if (nodes_.empty()) {
        const_cast<ClusterConfig*>(this)->initDefaultConfig();
    }
    
    std::vector<NodeInfo> masters;
    for (const auto& node : nodes_) {
        if (node.role == "master" && node.is_healthy) {
            masters.push_back(node);
        }
    }
    return masters;
}

std::vector<NodeInfo> ClusterConfig::getAllNodes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return nodes_;
}

size_t ClusterConfig::getNodeCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return nodes_.size();
}

void ClusterConfig::markNodeUnhealthy(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& node : nodes_) {
        if (node.id == node_id) {
            node.is_healthy = false;
            std::cout << "[Cluster] 标记节点 " << node_id << " 为不健康" << std::endl;
            break;
        }
    }
}

void ClusterConfig::markNodeHealthy(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& node : nodes_) {
        if (node.id == node_id) {
            node.is_healthy = true;
            std::cout << "[Cluster] 标记节点 " << node_id << " 为健康" << std::endl;
            break;
        }
    }
}
