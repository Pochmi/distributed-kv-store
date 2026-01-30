// src/network/simple_server.h
#ifndef SIMPLE_SERVER_H
#define SIMPLE_SERVER_H

#include <functional>
#include <string>
#include <thread>
#include <vector>
#include <atomic>
#include <memory>

class KVStore;  // 前向声明

class SimpleServer {
public:
    using RequestHandler = std::function<std::string(const std::string&)>;
    
    SimpleServer(int port, std::shared_ptr<KVStore> store);
    ~SimpleServer();
    
    bool Start();
    void Stop();
    bool IsRunning() const { return running_; }
    
private:
    void Run();
    void HandleClient(int client_fd);
    std::string ProcessCommand(const std::string& request);
    
    int port_;
    int server_fd_;
    std::atomic<bool> running_;
    std::shared_ptr<KVStore> store_;
    std::vector<std::thread> worker_threads_;
};

#endif // SIMPLE_SERVER_H
