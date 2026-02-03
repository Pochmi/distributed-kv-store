#include "connection.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <iostream>

Connection::Connection() : sockfd_(-1) {}
Connection::~Connection() { close(); }

bool Connection::connect(const std::string& host, int port) {
    if (sockfd_ >= 0) close();
    
    sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_ < 0) {
        std::cerr << "Failed to create socket: " << strerror(errno) << std::endl;
        return false;
    }
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
        std::cerr << "Invalid address: " << host << std::endl;
        close();
        return false;
    }
    
    if (::connect(sockfd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Failed to connect to " << host << ":" << port 
                  << " - " << strerror(errno) << std::endl;
        close();
        return false;
    }
    
    return true;
}

bool Connection::send(const std::string& data) {
    if (sockfd_ < 0) {
        std::cerr << "Socket not connected" << std::endl;
        return false;
    }
    
    ssize_t bytes_sent = ::send(sockfd_, data.c_str(), data.length(), 0);
    if (bytes_sent < 0) {
        std::cerr << "Failed to send data: " << strerror(errno) << std::endl;
        return false;
    }
    
    return bytes_sent == static_cast<ssize_t>(data.length());
}

bool Connection::receive(std::string& data) {
    if (sockfd_ < 0) {
        std::cerr << "Socket not connected" << std::endl;
        return false;
    }
    
    char buffer[4096];
    ssize_t bytes_recv = ::recv(sockfd_, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_recv < 0) {
        std::cerr << "Failed to receive data: " << strerror(errno) << std::endl;
        return false;
    } else if (bytes_recv == 0) {
        std::cout << "Connection closed by server" << std::endl;
        return false;
    }
    
    buffer[bytes_recv] = '\0';
    data = std::string(buffer, bytes_recv);
    return true;
}

void Connection::close() {
    if (sockfd_ >= 0) {
        ::close(sockfd_);
        sockfd_ = -1;
    }
}
