#include "cluster_config.h"
#include <functional>
#include <iostream>
#include <cstdlib>

ClusterConfig& ClusterConfig::getInstance() {
    static ClusterConfig instance;
    return instance;
}

void ClusterConfig::initDefaultConfig() {
    // 单节点配置 - 总是使用6381端口
    // 可以通过环境变量 KV_SERVER_PORT 修改
    const char* env_port = std::getenv("KV_SERVER_PORT");
    int port = env_port ? std::atoi(env_port) : 6381;
    
    nodes_ = {
        {"server-1", "127.0.0.1", port, "master", true}
    };
    
    std::cout << "[Cluster] Config: single node at 127.0.0.1:" << port << std::endl;
    std::cout << "[Cluster] Hint: Start server with: ./build/kv_server " << port << std::endl;
}

NodeInfo ClusterConfig::getNodeByKey(const std::string& key) {
    if (nodes_.empty()) {
        initDefaultConfig();
    }
    
    // 单节点：总是返回这个节点
    return nodes_[0];
}
