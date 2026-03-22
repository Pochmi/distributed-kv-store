#include "heartbeat.h"
#include "../common/logger.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <chrono>
#include <sstream>

HeartbeatManager::HeartbeatManager(const std::string& node_id, int port)
    : node_id_(node_id)
    , heartbeat_port_(port + 100)
    , interval_ms_(1000)
    , timeout_ms_(3000)
    , running_(false) {
    LOG_INFO("HeartbeatManager created for node " + node_id + " on port " + std::to_string(heartbeat_port_));
}

HeartbeatManager::~HeartbeatManager() {
    stop();
}

void HeartbeatManager::start(int interval_ms) {
    if (running_) return;
    interval_ms_ = interval_ms;
    running_ = true;
    
    // 发送线程
    send_thread_ = std::thread([this]() {
        while (running_) {
            sendLoop();
            std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms_));
        }
    });
    
    // 接收线程
    recv_thread_ = std::thread([this]() {
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) {
            LOG_ERROR("Failed to create heartbeat socket");
            return;
        }
        
        int opt = 1;
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(heartbeat_port_);
        addr.sin_addr.s_addr = INADDR_ANY;
        
        if (bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
            LOG_ERROR("Failed to bind heartbeat port " + std::to_string(heartbeat_port_));
            close(sock);
            return;
        }
        
        LOG_INFO("Heartbeat listening on port " + std::to_string(heartbeat_port_));
        
        char buffer[256];
        while (running_) {
            sockaddr_in from;
            socklen_t from_len = sizeof(from);
            int n = recvfrom(sock, buffer, sizeof(buffer)-1, 0, (sockaddr*)&from, &from_len);
            if (n > 0) {
                buffer[n] = '\0';
                std::string msg(buffer);
                if (msg.substr(0, 10) == "HEARTBEAT:") {
                    std::string from_node = msg.substr(10);
                    recordHeartbeat(from_node);
                    if (callback_) {
                        callback_(from_node, true);
                    }
                }
            }
        }
        close(sock);
    });
    
    LOG_INFO("Heartbeat started");
}

void HeartbeatManager::stop() {
    if (!running_) return;
    running_ = false;
    if (send_thread_.joinable()) send_thread_.join();
    if (recv_thread_.joinable()) recv_thread_.join();
    LOG_INFO("Heartbeat stopped");
}

void HeartbeatManager::sendLoop() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& pair : nodes_) {
        const auto& node = pair.second;
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) continue;
        
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(node.port + 100);
        inet_pton(AF_INET, node.host.c_str(), &addr.sin_addr);
        
        std::string msg = "HEARTBEAT:" + node_id_;
        sendto(sock, msg.c_str(), msg.length(), 0, (sockaddr*)&addr, sizeof(addr));
        close(sock);
    }
}

void HeartbeatManager::recordHeartbeat(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = nodes_.find(node_id);
    if (it != nodes_.end()) {
        it->second.alive = true;
        it->second.missed_count = 0;
        it->second.last_heartbeat_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
}

bool HeartbeatManager::isAlive(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = nodes_.find(node_id);
    if (it == nodes_.end()) return false;
    return it->second.alive;
}

void HeartbeatManager::addNode(const std::string& node_id, const std::string& host, int port) {
    std::lock_guard<std::mutex> lock(mutex_);
    NodeStatus node;
    node.host = host;
    node.port = port;
    node.alive = true;
    node.last_heartbeat_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    node.missed_count = 0;
    nodes_[node_id] = node;
    LOG_INFO("Added node to heartbeat: " + node_id + " at " + host + ":" + std::to_string(port));
}

void HeartbeatManager::checkTimeout() {
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& pair : nodes_) {
        auto& node = pair.second;
        if (node.alive && (now - node.last_heartbeat_ms) > timeout_ms_) {
            node.missed_count++;
            if (node.missed_count >= 3) {
                node.alive = false;
                LOG_WARNING("Node " + pair.first + " marked as FAILED");
                if (callback_) {
                    callback_(pair.first, false);
                }
            }
        }
    }
}

std::vector<std::string> HeartbeatManager::getFailedNodes() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> failed;
    for (const auto& pair : nodes_) {
        if (!pair.second.alive) {
            failed.push_back(pair.first);
        }
    }
    return failed;
}

void HeartbeatManager::setCallback(Callback cb) {
    callback_ = cb;
}

std::string HeartbeatManager::getStatus() const {
    // 注意：因为 mutex_ 是 mutable，所以可以用 const_cast
    // 或者直接使用 const_cast
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mutex_));
    std::string status = "Heartbeat Status:\n";
    for (const auto& pair : nodes_) {
        status += "  " + pair.first + ": " + pair.second.host + ":" + 
                  std::to_string(pair.second.port) + " [" +
                  (pair.second.alive ? "ALIVE" : "DEAD") + "]\n";
    }
    return status;
}