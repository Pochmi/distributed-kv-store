// src/main_server.cc
#include "core/kv_store.h"
#include "network/simple_server.h"
#include "common/logger.h"
#include <iostream>
#include <memory>
#include <signal.h>

std::unique_ptr<SimpleServer> server;

void signal_handler(int signal) {
    if (server) {
        std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
        server->Stop();
    }
}

int main(int argc, char* argv[]) {
    // 设置日志级别
    Logger::instance().set_level(INFO);
    
    // 注册信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    std::cout << "=== Distributed KV Store - Single Node Server ===" << std::endl;
    std::cout << "Starting server..." << std::endl;
    
    // 创建存储实例
    auto store = KVStore::CreateMemoryStore();
    
    // 创建并启动服务器
    int port = 6379;  // 默认使用Redis端口
    if (argc > 1) {
        port = std::stoi(argv[1]);
    }
    
    server = std::make_unique<SimpleServer>(port, std::move(store));
    
    if (!server->Start()) {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }
    
    std::cout << "Server is running on port " << port << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  SET <key> <value>" << std::endl;
    std::cout << "  GET <key>" << std::endl;
    std::cout << "  DEL <key>" << std::endl;
    std::cout << "  EXISTS <key>" << std::endl;
    std::cout << "  PING" << std::endl;
    std::cout << "  QUIT" << std::endl;
    std::cout << "Press Ctrl+C to stop server" << std::endl;
    
    // 等待服务器停止
    while (server->IsRunning()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    std::cout << "Server stopped" << std::endl;
    return 0;
}
