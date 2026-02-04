#ifndef ROUTER_H
#define ROUTER_H

#include "cluster_config.h"
#include <string>
#include <memory>

class Router {
public:
    Router();
    
    // 路由key到对应的节点
    NodeInfo route(const std::string& key);
    
    // 计算key的哈希值
    uint32_t hash(const std::string& key);
    
    // 获取所有节点
    std::vector<NodeInfo> getAllNodes();
    
    // 更新节点状态
    void markNodeUnhealthy(const std::string& node_id);
    void markNodeHealthy(const std::string& node_id);
    
private:
    ClusterConfig& config_;
};

#endif
