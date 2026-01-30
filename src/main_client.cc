// src/main_client.cc
#include "common/logger.h"
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

class SimpleClient {
public:
    SimpleClient(const std::string& host, int port) : host_(host), port_(port), sock_fd_(-1) {}
    
    bool Connect() {
        sock_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (sock_fd_ < 0) {
            std::cerr << "Failed to create socket" << std::endl;
            return false;
        }
        
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port_);
        
        if (inet_pton(AF_INET, host_.c_str(), &server_addr.sin_addr) <= 0) {
            std::cerr << "Invalid address" << std::endl;
            close(sock_fd_);
            return false;
        }
        
        if (connect(sock_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            std::cerr << "Connection failed" << std::endl;
            close(sock_fd_);
            return false;
        }
        
        std::cout << "Connected to " << host_ << ":" << port_ << std::endl;
        return true;
    }
    
    std::string SendCommand(const std::string& command) {
        if (sock_fd_ < 0) {
            return "ERROR: Not connected";
        }
        
        std::string full_command = command + "\n";
        if (send(sock_fd_, full_command.c_str(), full_command.length(), 0) < 0) {
            return "ERROR: Send failed";
        }
        
        char buffer[1024];
        ssize_t bytes_read = recv(sock_fd_, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read <= 0) {
            return "ERROR: Receive failed";
        }
        
        buffer[bytes_read] = '\0';
        return std::string(buffer);
    }
    
    void Disconnect() {
        if (sock_fd_ >= 0) {
            close(sock_fd_);
            sock_fd_ = -1;
        }
    }
    
    ~SimpleClient() {
        Disconnect();
    }
    
private:
    std::string host_;
    int port_;
    int sock_fd_;
};

int main(int argc, char* argv[]) {
    Logger::instance().set_level(WARNING);  // 客户端日志较少
    
    std::string host = "127.0.0.1";
    int port = 6379;
    
    if (argc > 1) {
        host = argv[1];
    }
    if (argc > 2) {
        port = std::stoi(argv[2]);
    }
    
    SimpleClient client(host, port);
    
    if (!client.Connect()) {
        return 1;
    }
    
    std::cout << "KV Store Client" << std::endl;
    std::cout << "Type 'quit' to exit" << std::endl;
    std::cout << "Commands: SET <key> <value>, GET <key>, DEL <key>, EXISTS <key>, PING" << std::endl;
    
    std::string input;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, input);
        
        if (input == "quit" || input == "exit") {
            break;
        }
        
        std::string response = client.SendCommand(input);
        std::cout << response;
    }
    
    client.Disconnect();
    return 0;
}
