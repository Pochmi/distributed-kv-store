#include "router.h"
#include <iostream>
#include <functional>

Router::Router() : config_(ClusterConfig::getInstance()) {
    std::cout << "[Router] 路由器初始化完成" << std::endl;
}

uint32_t Router::hash(const std::string& key) {
    std::hash<std::string> hash_fn;
    return static_cast<uint32_t>(hash_fn(key));
}

NodeInfo Router::route(const std::string& key) {
    uint32_t hash_value = hash(key);
    auto target_node = config_.getNodeByKey(key);
    
    std::cout << "[Router] 键 '" << key << "' 哈希值: " << hash_value 
              << " -> " << target_node.id << " (" << target_node.address() << ")" << std::endl;
    
    return target_node;
}

std::vector<NodeInfo> Router::getAllNodes() {
    return config_.getAllNodes();
}

void Router::markNodeUnhealthy(const std::string& node_id) {
    config_.markNodeUnhealthy(node_id);
    std::cout << "[Router] 标记节点 " << node_id << " 为不可用" << std::endl;
}

void Router::markNodeHealthy(const std::string& node_id) {
    config_.markNodeHealthy(node_id);
    std::cout << "[Router] 标记节点 " << node_id << " 为可用" << std::endl;
}
