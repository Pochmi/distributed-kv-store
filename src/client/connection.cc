#include "connection.h"
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>

Connection::Connection(const std::string& host, int port) 
    : host_(host), port_(port), sockfd_(-1), connected_(false) {}

Connection::~Connection() {
    disconnect();
}

bool Connection::createSocket() {
    sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_ < 0) {
        std::cerr << "[Connection] 创建socket失败" << std::endl;
        return false;
    }
    
    // 设置非阻塞模式（可选）
    // int flags = fcntl(sockfd_, F_GETFL, 0);
    // fcntl(sockfd_, F_SETFL, flags | O_NONBLOCK);
    
    return true;
}

bool Connection::connect() {
    if (connected_) {
        return true;
    }
    
    if (!createSocket()) {
        return false;
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);
    
    if (inet_pton(AF_INET, host_.c_str(), &server_addr.sin_addr) <= 0) {
        std::cerr << "[Connection] 无效的地址: " << host_ << std::endl;
        close(sockfd_);
        return false;
    }
    
    // 设置连接超时
    struct timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
    
    setsockopt(sockfd_, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    setsockopt(sockfd_, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    
    std::cout << "[Connection] 连接到 " << host_ << ":" << port_ << "..." << std::endl;
    
    if (::connect(sockfd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "[Connection] 连接失败: " << strerror(errno) << std::endl;
        close(sockfd_);
        sockfd_ = -1;
        return false;
    }
    
    connected_ = true;
    std::cout << "[Connection] 连接成功" << std::endl;
    return true;
}

void Connection::disconnect() {
    if (sockfd_ >= 0) {
        close(sockfd_);
        sockfd_ = -1;
        connected_ = false;
        std::cout << "[Connection] 断开连接" << std::endl;
    }
}

bool Connection::send(const std::string& data) {
    if (!connected_ && !connect()) {
        return false;
    }
    
    size_t total_sent = 0;
    while (total_sent < data.length()) {
        ssize_t sent = write(sockfd_, data.c_str() + total_sent, data.length() - total_sent);
        if (sent < 0) {
            std::cerr << "[Connection] 发送失败: " << strerror(errno) << std::endl;
            disconnect();
            return false;
        }
        total_sent += sent;
    }
    
    return true;
}

std::string Connection::receive() {
    if (!connected_) {
        throw std::runtime_error("未连接");
    }
    
    char buffer[4096];
    std::string result;
    
    // 设置接收超时
    fd_set readfds;
    struct timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
    
    FD_ZERO(&readfds);
    FD_SET(sockfd_, &readfds);
    
    int ready = select(sockfd_ + 1, &readfds, NULL, NULL, &timeout);
    
    if (ready > 0) {
        ssize_t received = read(sockfd_, buffer, sizeof(buffer) - 1);
        if (received > 0) {
            buffer[received] = '\0';
            result = buffer;
        } else if (received == 0) {
            disconnect();  // 连接关闭
        } else {
            std::cerr << "[Connection] 接收失败: " << strerror(errno) << std::endl;
            disconnect();
        }
    } else if (ready == 0) {
        std::cout << "[Connection] 接收超时" << std::endl;
    }
    
    return result;
}

bool Connection::isConnected() const {
    return connected_;
}
