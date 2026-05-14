#ifndef CLIENT_ROUTER_H
#define CLIENT_ROUTER_H

#include "cluster_config.h"
#include <string>
#include <vector>

class Router {
public:
    Router();
    NodeInfo route(const std::string& key);
    std::vector<NodeInfo> getAllMasters();
    
private:
    void refreshHashRing();
    ClusterConfig& config_;
};

#endif
