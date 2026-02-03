#include "cluster_config.h"
#include <functional>
#include <iostream>

ClusterConfig& ClusterConfig::getInstance() {
    static ClusterConfig instance;
    return instance;
}

void ClusterConfig::initDefaultConfig() {
    nodes_ = {
        {"shard-1", "127.0.0.1", 6381, "master", true},
        {"shard-2", "127.0.0.1", 6382, "master", true},
        {"shard-3", "127.0.0.1", 6383, "master", true}
    };
    
    std::cout << "[Cluster] Initialized with " << nodes_.size() << " nodes" << std::endl;
}

NodeInfo ClusterConfig::getNodeByKey(const std::string& key) {
    if (nodes_.empty()) initDefaultConfig();
    
    std::hash<std::string> hash_fn;
    size_t hash_value = hash_fn(key);
    size_t node_index = hash_value % nodes_.size();
    
    return nodes_[node_index];
}
