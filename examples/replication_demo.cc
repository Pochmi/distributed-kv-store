#include "client/kv_client.h"
#include "common/logger.h"
#include <iostream>
#include <thread>

void testReplication() {
    Logger::info("Starting replication test...");
    
    // 创建客户端连接到主节点
    KVClient client("127.0.0.1", 6380);
    
    if (!client.connect()) {
        Logger::error("Failed to connect to master");
        return;
    }
    
    // 写入数据到主节点
    for (int i = 0; i < 10; i++) {
        std::string key = "key_" + std::to_string(i);
        std::string value = "value_" + std::to_string(i);
        
        if (client.put(key, value)) {
            Logger::info("Write to master successful: %s = %s", 
                        key.c_str(), value.c_str());
        } else {
            Logger::error("Write failed: %s", key.c_str());
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // 从从节点读取数据验证复制
    KVClient slave1_client("127.0.0.1", 6381);
    if (slave1_client.connect()) {
        std::string value;
        if (slave1_client.get("key_5", value)) {
            Logger::info("Read from slave1: key_5 = %s", value.c_str());
        }
    }
    
    KVClient slave2_client("127.0.0.1", 6382);
    if (slave2_client.connect()) {
        std::string value;
        if (slave2_client.get("key_5", value)) {
            Logger::info("Read from slave2: key_5 = %s", value.c_str());
        }
    }
    
    Logger::info("Replication test completed");
}

int main() {
    Logger::setLevel(LogLevel::INFO);
    testReplication();
    return 0;
}
