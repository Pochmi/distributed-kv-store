#ifndef CLIENT_ROUTER_H
#define CLIENT_ROUTER_H

#include "cluster_config.h"
#include <string>

class Router {
public:
    Router();
    NodeInfo route(const std::string& key);
    
private:
    ClusterConfig& config_;
    uint32_t hash(const std::string& key);
};
#endif
