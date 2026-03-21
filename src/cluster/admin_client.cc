#include "admin_client.h"
#include "../common/logger.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <sstream>

AdminCommandClient::AdminCommandClient(const std::string& master_addr) 
    : master_addr_(master_addr), socket_fd_(-1), connected_(false) {
    if (!master_addr_.empty()) {
        connect(master_addr_);
    }
}

AdminCommandClient::~AdminCommandClient() {
    disconnect();
}

bool AdminCommandClient::connect(const std::string& master_addr) {
    master_addr_ = master_addr;
    
    // 解析地址 (格式: host:port)
    size_t colon_pos = master_addr_.find(':');
    if (colon_pos == std::string::npos) {
        last_error_ = "Invalid address format. Use host:port";
        Logger::instance().error(last_error_);
        return false;
    }
    
    std::string host = master_addr_.substr(0, colon_pos);
    int port = std::stoi(master_addr_.substr(colon_pos + 1));
    
    // 创建socket
    socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd_ < 0) {
        last_error_ = "Failed to create socket";
        Logger::instance().error(last_error_);
        return false;
    }
    
    // 连接服务器
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr) <= 0) {
        last_error_ = "Invalid address";
        Logger::instance().error(last_error_);
        close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }
    
    if (::connect(socket_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        last_error_ = "Connection failed";
        Logger::instance().error(last_error_);
        close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }
    
    connected_ = true;
    Logger::instance().info("Connected to master at " + master_addr_);
    return true;
}

void AdminCommandClient::disconnect() {
    if (socket_fd_ != -1) {
        close(socket_fd_);
        socket_fd_ = -1;
    }
    connected_ = false;
    Logger::instance().info("Disconnected from master");
}

bool AdminCommandClient::sendCommand(const std::string& cmd, std::string& response) {
    if (!connected_) {
        last_error_ = "Not connected";
        return false;
    }
    
    // 发送命令
    std::string cmd_with_newline = cmd + "\n";
    ssize_t sent = send(socket_fd_, cmd_with_newline.c_str(), cmd_with_newline.length(), 0);
    if (sent <= 0) {
        last_error_ = "Failed to send command";
        return false;
    }
    
    // 接收响应
    char buffer[4096];
    ssize_t received = recv(socket_fd_, buffer, sizeof(buffer) - 1, 0);
    if (received <= 0) {
        last_error_ = "Failed to receive response";
        return false;
    }
    
    buffer[received] = '\0';
    response = buffer;
    return true;
}

std::vector<NodeStatus> AdminCommandClient::listNodes() {
    std::vector<NodeStatus> nodes;
    std::string response;
    
    if (sendCommand("LIST_NODES", response)) {
        // 解析响应 (简化版)
        Logger::instance().info("Received node list");
    } else {
        Logger::instance().error("Failed to list nodes: " + last_error_);
    }
    
    return nodes;
}

ClusterStats AdminCommandClient::getStats() {
    ClusterStats stats = {0, 0, 0, "", {}};
    std::string response;
    
    if (sendCommand("STATS", response)) {
        Logger::instance().info("Received stats");
    } else {
        Logger::instance().error("Failed to get stats: " + last_error_);
    }
    
    return stats;
}

bool AdminCommandClient::initiateFailover() {
    std::string response;
    if (sendCommand("FAILOVER", response)) {
        Logger::instance().info("Failover initiated");
        return true;
    } else {
        Logger::instance().error("Failed to initiate failover: " + last_error_);
        return false;
    }
}

bool AdminCommandClient::promoteSlave(const std::string& slave_id) {
    std::string response;
    std::string cmd = "PROMOTE " + slave_id;
    if (sendCommand(cmd, response)) {
        Logger::instance().info("Promoted slave " + slave_id);
        return true;
    } else {
        Logger::instance().error("Failed to promote slave " + slave_id + ": " + last_error_);
        return false;
    }
}

bool AdminCommandClient::demoteMaster(const std::string& master_id) {
    std::string response;
    std::string cmd = "DEMOTE " + master_id;
    if (sendCommand(cmd, response)) {
        Logger::instance().info("Demoted master " + master_id);
        return true;
    } else {
        Logger::instance().error("Failed to demote master " + master_id + ": " + last_error_);
        return false;
    }
}
