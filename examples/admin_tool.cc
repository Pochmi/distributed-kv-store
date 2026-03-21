#include "../src/common/logger.h"
#include "../src/client/cluster_config.h"
#include <iostream>
#include <memory>

int main() {
    // 初始化日志
    Logger::instance().setLevel(LOG_INFO);
    Logger::instance().info("Admin tool started");
    
    // 获取集群配置实例
    ClusterConfig& config = ClusterConfig::getInstance();
    
    // 加载配置文件（如果有）
    // config.loadFromFile("config.json");
    
    // 显示集群信息
    std::cout << "Cluster Information:" << std::endl;
    auto nodes = config.getAllNodes();
    for (const auto& node : nodes) {
        std::cout << "  Node: " << node.id 
                  << " at " << node.host << ":" << node.port
                  << " (role: " << node.role << ")"
                  << std::endl;
    }
    
    Logger::instance().info("Admin tool finished");
    return 0;
}
