#include "router.h"
#include "../common/consistent_hash.h"
#include <iostream>

static ConsistentHash g_hash_ring;

Router::Router() : config_(ClusterConfig::getInstance()) {
    refreshHashRing();
}

void Router::refreshHashRing() {
    auto masters = config_.getAvailableMasters();
    std::vector<ConsistentHash::Node> nodes;
    for (const auto& node : masters) {
        nodes.push_back(ConsistentHash::Node(node.id, node.host, node.port, node.role));
    }
    g_hash_ring.updateNodes(nodes);
}

NodeInfo Router::route(const std::string& key) {
    auto masters = config_.getAvailableMasters();
    if (masters.empty()) {
        throw std::runtime_error("No available master nodes");
    }
    
    ConsistentHash::Node target = g_hash_ring.getNode(key);
    
    NodeInfo result;
    result.id = target.id;
    result.host = target.host;
    result.port = target.port;
    result.role = target.role;
    result.is_healthy = true;
    
    std::cout << "[Router] 键 '" << key << "' -> " << result.id 
              << " (" << result.address() << ")" << std::endl;
    return result;
}

std::vector<NodeInfo> Router::getAllMasters() {
    return config_.getAvailableMasters();
}
