#include <iostream>
#include <string>
#include "client/kv_client.h"

void printUsage() {
    std::cout << "分布式KV存储客户端 - 使用说明" << std::endl;
    std::cout << "=============================" << std::endl;
    std::cout << "命令格式:" << std::endl;
    std::cout << "  kv_client set <key> <value>  # 设置键值" << std::endl;
    std::cout << "  kv_client get <key>          # 获取键值" << std::endl;
    std::cout << "  kv_client del <key>          # 删除键值" << std::endl;
    std::cout << "  kv_client test               # 运行测试" << std::endl;
    std::cout << std::endl;
    std::cout << "示例:" << std::endl;
    std::cout << "  ./kv_client set name \"张三\"" << std::endl;
    std::cout << "  ./kv_client get name" << std::endl;
    std::cout << "  ./kv_client del name" << std::endl;
}

void runTest() {
    std::cout << "=== 运行客户端测试 ===" << std::endl;
    
    KVClient client;
    
    // 测试1: SET
    std::cout << "\n1. 测试 SET 操作:" << std::endl;
    if (client.put("test_key", "test_value")) {
        std::cout << "   ✅ SET 成功" << std::endl;
    } else {
        std::cout << "   ❌ SET 失败" << std::endl;
    }
    
    // 测试2: GET
    std::cout << "\n2. 测试 GET 操作:" << std::endl;
    std::string value = client.get("test_key");
    if (!value.empty()) {
        std::cout << "   ✅ GET 成功: " << value << std::endl;
    } else {
        std::cout << "   ❌ GET 失败" << std::endl;
    }
    
    // 测试3: DELETE
    std::cout << "\n3. 测试 DELETE 操作:" << std::endl;
    if (client.del("test_key")) {
        std::cout << "   ✅ DELETE 成功" << std::endl;
    } else {
        std::cout << "   ❌ DELETE 失败" << std::endl;
    }
    
    // 测试4: 验证DELETE
    std::cout << "\n4. 验证 DELETE 结果:" << std::endl;
    value = client.get("test_key");
    if (value.empty()) {
        std::cout << "   ✅ 键已删除" << std::endl;
    } else {
        std::cout << "   ❌ 键仍然存在: " << value << std::endl;
    }
    
    std::cout << "\n=== 测试完成 ===" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage();
        return 1;
    }
    
    std::string command = argv[1];
    KVClient client;
    
    if (command == "set" && argc >= 4) {
        std::string key = argv[2];
        std::string value = argv[3];
        
        if (client.put(key, value)) {
            std::cout << "✅ SET 成功: " << key << " = " << value << std::endl;
        } else {
            std::cerr << "❌ SET 失败" << std::endl;
        }
        
    } else if (command == "get" && argc >= 3) {
        std::string key = argv[2];
        std::string value = client.get(key);
        
        if (!value.empty()) {
            std::cout << key << " = " << value << std::endl;
        } else {
            std::cout << "键 '" << key << "' 不存在" << std::endl;
        }
        
    } else if (command == "del" && argc >= 3) {
        std::string key = argv[2];
        
        if (client.del(key)) {
            std::cout << "✅ DELETE 成功: " << key << std::endl;
        } else {
            std::cerr << "❌ DELETE 失败" << std::endl;
        }
        
    } else if (command == "test") {
        runTest();
        
    } else if (command == "--help" || command == "-h") {
        printUsage();
        
    } else {
        std::cerr << "错误: 无效的命令或参数" << std::endl;
        printUsage();
        return 1;
    }
    
    return 0;
}
