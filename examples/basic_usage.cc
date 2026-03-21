#include "../src/common/logger.h"
#include "../src/client/kv_client.h"
#include <iostream>
#include <memory>

int main() {
    // 初始化日志
    Logger::instance().setLevel(LOG_INFO);
    Logger::instance().info("Basic usage example started");
    
    // 创建客户端
    auto client = std::make_unique<KVClient>();
    
    // 执行一些操作
    std::cout << "Putting key1 = value1" << std::endl;
    if (client->put("key1", "value1")) {
        std::cout << "Put successful" << std::endl;
    } else {
        std::cout << "Put failed" << std::endl;
    }
    
    std::cout << "Getting key1" << std::endl;
    std::string value = client->get("key1");
    if (!value.empty()) {
        std::cout << "Got: " << value << std::endl;
    } else {
        std::cout << "Key not found" << std::endl;
    }
    
    std::cout << "Deleting key1" << std::endl;
    if (client->del("key1")) {
        std::cout << "Delete successful" << std::endl;
    } else {
        std::cout << "Delete failed" << std::endl;
    }
    
    // 测试 ping
    if (client->ping()) {
        std::cout << "Server is reachable" << std::endl;
    } else {
        std::cout << "Server is not reachable" << std::endl;
    }
    
    Logger::instance().info("Basic usage example finished");
    return 0;
}
