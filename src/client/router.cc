#include "router.h"
#include <iostream>
#include <functional>

Router::Router() : config_(ClusterConfig::getInstance()) {
    config_.initDefaultConfig();
}

uint32_t Router::hash(const std::string& key) {
    return std::hash<std::string>{}(key);
}

NodeInfo Router::route(const std::string& key) {
    uint32_t hash_value = hash(key);
    auto all_nodes = config_.getAllNodes();
    
    if (all_nodes.empty()) {
        throw std::runtime_error("No nodes available in cluster");
    }
    
    size_t node_index = hash_value % all_nodes.size();
    NodeInfo target_node = all_nodes[node_index];
    
    std::cout << "[Router] Key '" << key << "' -> Node " 
              << target_node.id << " (" << target_node.address() << ")" << std::endl;
    
    return target_node;
}
