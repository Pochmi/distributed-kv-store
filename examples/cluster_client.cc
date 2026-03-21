#include "../src/common/logger.h"
#include "../src/client/kv_client.h"
#include "../src/client/cluster_config.h"
#include <iostream>
#include <memory>
#include <vector>

int main() {
    // 初始化日志
    Logger::instance().setLevel(LOG_INFO);
    Logger::instance().info("Cluster client example started");
    
    // 获取集群配置
    ClusterConfig& config = ClusterConfig::getInstance();
    
    // 假设配置已经加载
    std::vector<NodeInfo> nodes = config.getAllNodes();
    
    if (nodes.empty()) {
        std::cout << "No nodes in cluster, using default configuration" << std::endl;
    } else {
        std::cout << "Found " << nodes.size() << " nodes in cluster" << std::endl;
        for (const auto& node : nodes) {
            std::cout << "  Node: " << node.id 
                      << " at " << node.host << ":" << node.port
                      << " (role: " << node.role << ")"
                      << std::endl;
        }
    }
    
    // 创建客户端（将使用 Router 自动路由到正确的节点）
    auto client = std::make_unique<KVClient>();
    
    // 执行操作
    std::cout << "Putting cluster_key = cluster_value" << std::endl;
    if (client->put("cluster_key", "cluster_value")) {
        std::cout << "Put successful" << std::endl;
    } else {
        std::cout << "Put failed" << std::endl;
    }
    
    std::string value = client->get("cluster_key");
    if (!value.empty()) {
        std::cout << "Got: " << value << std::endl;
    } else {
        std::cout << "Key not found" << std::endl;
    }
    
    Logger::instance().info("Cluster client example finished");
    return 0;
}
