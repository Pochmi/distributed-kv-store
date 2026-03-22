#include "admin_client.h"
#include "../common/logger.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <sstream>

AdminClient::AdminClient(const std::string& host, int port)
    : host_(host), port_(port) {
    LOG_INFO("AdminClient created for " + host + ":" + std::to_string(port));
}

AdminClient::~AdminClient() = default;

int AdminClient::connect() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        LOG_ERROR("Failed to create socket");
        return -1;
    }
    
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    inet_pton(AF_INET, host_.c_str(), &addr.sin_addr);
    
    if (::connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        LOG_ERROR("Failed to connect to " + host_ + ":" + std::to_string(port_));
        close(sock);
        return -1;
    }
    
    return sock;
}

void AdminClient::disconnect(int sock) {
    if (sock >= 0) close(sock);
}

std::string AdminClient::sendCommand(const std::string& cmd) {
    return sendCommand(cmd, {});
}

std::string AdminClient::sendCommand(const std::string& cmd, const std::vector<std::string>& args) {
    int sock = connect();
    if (sock < 0) return "ERROR: Connection failed";
    
    // 构建命令
    std::string full_cmd = cmd;
    for (const auto& arg : args) {
        full_cmd += " " + arg;
    }
    full_cmd += "\n";
    
    // 发送
    send(sock, full_cmd.c_str(), full_cmd.length(), 0);
    
    // 接收响应
    char buffer[4096];
    int n = recv(sock, buffer, sizeof(buffer)-1, 0);
    disconnect(sock);
    
    if (n <= 0) return "ERROR: No response";
    
    buffer[n] = '\0';
    return std::string(buffer);
}

std::string AdminClient::ping() {
    return sendCommand("PING");
}

std::string AdminClient::getStatus() {
    return sendCommand("STATUS");
}

std::string AdminClient::getNodes() {
    return sendCommand("NODES");
}

std::string AdminClient::failover(const std::string& target_node) {
    return sendCommand("FAILOVER", {target_node});
}

std::string AdminClient::help() {
    return sendCommand("HELP");
}