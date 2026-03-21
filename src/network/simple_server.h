#ifndef SIMPLE_SERVER_H
#define SIMPLE_SERVER_H

#include <memory>
#include <string>
#include <atomic>
#include <thread>
#include <functional>

class KVStore;

class SimpleServer {
public:
    // 请求处理器类型
    using RequestHandler = std::function<std::string(const std::string&)>;
    
    // 构造函数
    SimpleServer(int port, std::shared_ptr<KVStore> store);
    SimpleServer(int port, std::shared_ptr<KVStore> store, RequestHandler handler);
    ~SimpleServer();
    
    bool Start();
    void Stop();
    
    // 设置请求处理器
    void SetRequestHandler(RequestHandler handler) { request_handler_ = handler; }
    
private:
    void Run();
    void HandleClient(int client_fd);
    std::string ProcessCommand(const std::string& request);
    
    int port_;
    int server_fd_;
    std::atomic<bool> running_;
    std::shared_ptr<KVStore> store_;
    RequestHandler request_handler_;
};

#endif