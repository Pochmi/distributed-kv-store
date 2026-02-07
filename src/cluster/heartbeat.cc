#include "heartbeat.h"
#include "common/logger.h"

#include <thread>
#include <chrono>
#include <sstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

HeartbeatManager::HeartbeatManager(const std::string& node_id, 
                                 int interval_ms, 
                                 int timeout_ms)
    : node_id_(node_id)
    , interval_ms_(interval_ms)
    , timeout_ms_(timeout_ms)
    , running_(false) {
    Logger::info("HeartbeatManager initialized for node %s", node_id.c_str());
}

HeartbeatManager::~HeartbeatManager() {
    stop();
}

void HeartbeatManager::addNode(const std::string& node_id, 
                              const std::string& host, int port) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    NodeInfo node;
    node.id = node_id;
    node.host = host;
    node.port = port;
    node.is_alive = true;
    node.last_heartbeat_time = 0;
    node.missed_beats = 0;
    
    nodes_[node_id] = node;
    Logger::info("Added node %s at %s:%d to heartbeat monitor", 
                node_id.c_str(), host.c_str(), port);
}

void HeartbeatManager::removeNode(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = nodes_.find(node_id);
    if (it != nodes_.end()) {
        Logger::info("Removed node %s from heartbeat monitor", node_id.c_str());
        nodes_.erase(it);
    }
}

void HeartbeatManager::start() {
    if (running_) {
        return;
    }
    
    running_ = true;
    heartbeat_thread_ = std::thread(&HeartbeatManager::heartbeatThreadFunc, this);
    check_thread_ = std::thread(&HeartbeatManager::checkThreadFunc, this);
    
    Logger::info("HeartbeatManager started");
}

void HeartbeatManager::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    
    if (heartbeat_thread_.joinable()) {
        heartbeat_thread_.join();
    }
    
    if (check_thread_.joinable()) {
        check_thread_.join();
    }
    
    Logger::info("HeartbeatManager stopped");
}

HeartbeatStatus HeartbeatManager::getNodeStatus() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    HeartbeatStatus status;
    status.total_nodes = nodes_.size();
    
    for (const auto& kv : nodes_) {
        if (kv.second.is_alive) {
            status.alive_nodes++;
        } else {
            status.dead_nodes++;
        }
    }
    
    return status;
}

std::vector<std::string> HeartbeatManager::getDeadNodes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> dead_nodes;
    
    for (const auto& kv : nodes_) {
        if (!kv.second.is_alive) {
            dead_nodes.push_back(kv.first);
        }
    }
    
    return dead_nodes;
}

void HeartbeatManager::heartbeatThreadFunc() {
    Logger::info("Heartbeat thread started");
    
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms_));
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        // 发送心跳到所有节点
        for (auto& kv : nodes_) {
            sendHeartbeat(kv.second);
        }
    }
    
    Logger::info("Heartbeat thread stopped");
}

void HeartbeatManager::checkThreadFunc() {
    Logger::info("Heartbeat check thread started");
    
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms_ / 2));
        
        checkNodeHealth();
    }
    
    Logger::info("Heartbeat check thread stopped");
}

void HeartbeatManager::sendHeartbeat(NodeInfo& node) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        node.missed_beats++;
        return;
    }
    
    // 设置超时
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    
    // 连接到目标节点
    struct sockaddr_in node_addr;
    memset(&node_addr, 0, sizeof(node_addr));
    node_addr.sin_family = AF_INET;
    node_addr.sin_port = htons(node.port);
    
    if (inet_pton(AF_INET, node.host.c_str(), &node_addr.sin_addr) <= 0) {
        close(sockfd);
        node.missed_beats++;
        return;
    }
    
    if (connect(sockfd, (struct sockaddr*)&node_addr, sizeof(node_addr)) < 0) {
        close(sockfd);
        node.missed_beats++;
        return;
    }
    
    // 发送心跳请求
    std::string request = "HEARTBEAT " + node_id_;
    if (send(sockfd, request.c_str(), request.length(), 0) < 0) {
        close(sockfd);
        node.missed_beats++;
        return;
    }
    
    // 接收响应
    char buffer[256];
    int bytes = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    close(sockfd);
    
    if (bytes > 0) {
        buffer[bytes] = '\0';
        std::string response(buffer);
        
        if (response.find("PONG") == 0) {
            node.is_alive = true;
            node.missed_beats = 0;
            node.last_heartbeat_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count();
        }
    } else {
        node.missed_beats++;
    }
}

void HeartbeatManager::checkNodeHealth() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    for (auto& kv : nodes_) {
        auto& node = kv.second;
        
        // 检查是否超过超时时间
        if (node.last_heartbeat_time > 0) {
            auto elapsed = now - node.last_heartbeat_time;
            
            if (elapsed > timeout_ms_ && node.is_alive) {
                node.is_alive = false;
                Logger::warn("Node %s marked as dead (last heartbeat: %ldms ago)", 
                           node.id.c_str(), elapsed);
            } else if (elapsed <= timeout_ms_ && !node.is_alive) {
                node.is_alive = true;
                Logger::info("Node %s is alive again", node.id.c_str());
            }
        }
        
        // 检查连续丢失的心跳数
        if (node.missed_beats > 3 && node.is_alive) {
            node.is_alive = false;
            Logger::warn("Node %s marked as dead (missed %d beats)", 
                       node.id.c_str(), node.missed_beats);
        }
    }
}

std::string HeartbeatManager::getStatus() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::stringstream ss;
    ss << "Heartbeat Manager Status:\n";
    ss << "  Node ID: " << node_id_ << "\n";
    ss << "  Interval: " << interval_ms_ << "ms\n";
    ss << "  Timeout: " << timeout_ms_ << "ms\n";
    ss << "  Running: " << (running_ ? "true" : "false") << "\n";
    ss << "  Monitored nodes: " << nodes_.size() << "\n";
    
    int alive_count = 0;
    for (const auto& kv : nodes_) {
        if (kv.second.is_alive) alive_count++;
    }
    
    ss << "  Alive nodes: " << alive_count << "\n";
    ss << "  Dead nodes: " << (nodes_.size() - alive_count) << "\n";
    
    for (const auto& kv : nodes_) {
        const auto& node = kv.second;
        ss << "  - " << node.id << " [" << node.host << ":" << node.port 
           << "] alive=" << (node.is_alive ? "true" : "false")
           << " missed=" << node.missed_beats;
        
        if (node.last_heartbeat_time > 0) {
            auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count();
            ss << " last_beat=" << (now - node.last_heartbeat_time) << "ms ago";
        }
        ss << "\n";
    }
    
    return ss.str();
}
