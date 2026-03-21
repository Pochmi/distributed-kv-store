#include "../src/common/logger.h"
#include "../src/client/kv_client.h"
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

int main() {
    // 初始化日志
    Logger::instance().setLevel(LOG_INFO);
    Logger::instance().info("Replication demo started");
    
    // 创建客户端
    auto master_client = std::make_unique<KVClient>();
    auto slave_client = std::make_unique<KVClient>();
    
    // 注意：KVClient 默认会使用 Router 来路由请求
    // 这里我们假设已经配置了正确的节点信息
    // 在实际使用中，可能需要先设置路由配置
    
    std::cout << "Writing to master..." << std::endl;
    if (master_client->put("repl_key", "repl_value")) {
        std::cout << "Put successful" << std::endl;
    } else {
        std::cout << "Put failed" << std::endl;
    }
    
    // 等待复制
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    std::cout << "Reading from slave..." << std::endl;
    std::string value = slave_client->get("repl_key");
    if (!value.empty()) {
        std::cout << "Got from slave: " << value << std::endl;
    } else {
        std::cout << "Failed to read from slave or key not found" << std::endl;
    }
    
    // 删除操作
    std::cout << "Deleting key..." << std::endl;
    if (master_client->del("repl_key")) {
        std::cout << "Delete successful" << std::endl;
    } else {
        std::cout << "Delete failed" << std::endl;
    }
    
    // 测试 ping
    std::cout << "Testing connection..." << std::endl;
    if (master_client->ping()) {
        std::cout << "Master is reachable" << std::endl;
    } else {
        std::cout << "Master is not reachable" << std::endl;
    }
    
    Logger::instance().info("Replication demo finished");
    return 0;
}
