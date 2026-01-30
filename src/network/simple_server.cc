// src/network/simple_server.cc
#include "simple_server.h"
#include "../core/kv_store.h"
#include "../common/protocol.h"
#include "../common/logger.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>

SimpleServer::SimpleServer(int port, std::shared_ptr<KVStore> store) 
    : port_(port), server_fd_(-1), running_(false), store_(store) {}

SimpleServer::~SimpleServer() {
    Stop();
}

bool SimpleServer::Start() {
    // 创建socket
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        LOG_ERROR("Failed to create socket");
        return false;
    }
    
    // 设置SO_REUSEADDR选项
    int opt = 1;
    if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        LOG_ERROR("Failed to set socket options");
        close(server_fd_);
        return false;
    }
    
    // 绑定地址
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);
    
    if (bind(server_fd_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        LOG_ERROR("Failed to bind socket");
        close(server_fd_);
        return false;
    }
    
    // 开始监听
    if (listen(server_fd_, 10) < 0) {
        LOG_ERROR("Failed to listen on socket");
        close(server_fd_);
        return false;
    }
    
    running_ = true;
    
    // 启动服务器线程
    std::thread server_thread(&SimpleServer::Run, this);
    server_thread.detach();
    
    LOG_INFO("Server started on port " + std::to_string(port_));
    return true;
}

void SimpleServer::Stop() {
    if (running_) {
        running_ = false;
        if (server_fd_ >= 0) {
            close(server_fd_);
            server_fd_ = -1;
        }
        LOG_INFO("Server stopped");
    }
}

void SimpleServer::Run() {
    while (running_) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_fd = accept(server_fd_, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            if (running_) {
                LOG_ERROR("Failed to accept connection");
            }
            continue;
        }
        
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        LOG_INFO("New connection from " + std::string(client_ip) + 
                ":" + std::to_string(ntohs(client_addr.sin_port)));
        
        // 为每个客户端创建新线程处理
        std::thread client_thread(&SimpleServer::HandleClient, this, client_fd);
        client_thread.detach();
    }
}

void SimpleServer::HandleClient(int client_fd) {
    char buffer[1024];
    ssize_t bytes_read;
    
    while ((bytes_read = read(client_fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        std::string request(buffer);
        
        LOG_DEBUG("Received request: " + request);
        
        std::string response = ProcessCommand(request);
        
        LOG_DEBUG("Sending response: " + response);
        write(client_fd, response.c_str(), response.length());
    }
    
    close(client_fd);
    LOG_INFO("Client disconnected");
}

std::string SimpleServer::ProcessCommand(const std::string& request) {
    Request req = ProtocolParser::ParseRequest(request);
    Response resp;
    
    switch (req.type) {
        case CMD_SET:
            if (req.args.size() >= 2) {
                Status status = store_->Put(req.args[0], req.args[1]);
                resp.success = status.ok();
                resp.message = status.message;
            } else {
                resp.success = false;
                resp.message = "SET requires key and value";
            }
            break;
            
        case CMD_GET:
            if (req.args.size() >= 1) {
                std::string value;
                Status status = store_->Get(req.args[0], value);
                resp.success = status.ok();
                resp.message = status.message;
                if (status.ok()) {
                    resp.data = value;
                }
            } else {
                resp.success = false;
                resp.message = "GET requires key";
            }
            break;
            
        case CMD_DEL:
            if (req.args.size() >= 1) {
                Status status = store_->Delete(req.args[0]);
                resp.success = status.ok();
                resp.message = status.message;
            } else {
                resp.success = false;
                resp.message = "DEL requires key";
            }
            break;
            
        case CMD_EXISTS:
            if (req.args.size() >= 1) {
                Status status = store_->Contains(req.args[0]);
                resp.success = true;
                resp.message = status.ok() ? "true" : "false";
            } else {
                resp.success = false;
                resp.message = "EXISTS requires key";
            }
            break;
            
        case CMD_PING:
            resp.success = true;
            resp.message = "PONG";
            break;
            
        case CMD_QUIT:
            resp.success = true;
            resp.message = "BYE";
            break;
            
        default:
            resp.success = false;
            resp.message = "Unknown command";
            break;
    }
    
    return ProtocolParser::FormatResponse(resp);
}
