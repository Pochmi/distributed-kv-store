#include "heartbeat.h"
#include "../common/logger.h"
#include <chrono>
#include <thread>
#include <random>

HeartbeatManager::HeartbeatManager(const std::string& node_id, 
                                 int interval_ms, 
                                 int timeout_ms)
    : node_id_(node_id)
    , interval_ms_(interval_ms)
    , timeout_ms_(timeout_ms)
    , running_(false) {
    
    Logger::instance().debug("HeartbeatManager created for node: " + node_id);
}

HeartbeatManager::~HeartbeatManager() {
    stop();
}

void HeartbeatManager::addNode(const std::string& node_id, 
                              const std::string& host, int port) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    HeartbeatNodeInfo node_info;
    node_info.id = node_id;
    node_info.host = host;
    node_info.port = port;
    node_info.is_alive = true;
    node_info.last_heartbeat_time = 
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    node_info.missed_beats = 0;
    
    nodes_[node_id] = node_info;
    Logger::instance().info("Added node to heartbeat monitoring: " + node_id + 
                           " (" + host + ":" + std::to_string(port) + ")");
}

void HeartbeatManager::removeNode(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = nodes_.find(node_id);
    if (it != nodes_.end()) {
        nodes_.erase(it);
        Logger::instance().info("Removed node from heartbeat monitoring: " + node_id);
    } else {
        Logger::instance().warning("Node not found for removal: " + node_id);
    }
}

void HeartbeatManager::start() {
    if (running_) {
        Logger::instance().warning("HeartbeatManager already running");
        return;
    }
    
    running_ = true;
    heartbeat_thread_ = std::thread(&HeartbeatManager::heartbeatThreadFunc, this);
    check_thread_ = std::thread(&HeartbeatManager::checkThreadFunc, this);
    
    Logger::instance().info("HeartbeatManager started");
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
    
    Logger::instance().info("HeartbeatManager stopped");
}

HeartbeatStatus HeartbeatManager::getNodeStatus() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    HeartbeatStatus status;
    status.total_nodes = nodes_.size();
    status.alive_nodes = 0;
    status.dead_nodes = 0;
    
    for (const auto& pair : nodes_) {
        if (pair.second.is_alive) {
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
    for (const auto& pair : nodes_) {
        if (!pair.second.is_alive) {
            dead_nodes.push_back(pair.first);
        }
    }
    
    return dead_nodes;
}

std::string HeartbeatManager::getStatus() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string status = "HeartbeatManager Status:\n";
    status += "  Node ID: " + node_id_ + "\n";
    status += "  Interval: " + std::to_string(interval_ms_) + " ms\n";
    status += "  Timeout: " + std::to_string(timeout_ms_) + " ms\n";
    status += "  Running: " + std::string(running_ ? "yes" : "no") + "\n";
    status += "  Monitored nodes: " + std::to_string(nodes_.size()) + "\n";
    
    for (const auto& pair : nodes_) {
        status += "    " + pair.first + ": " + pair.second.host + ":" + 
                 std::to_string(pair.second.port) + 
                 " [" + (pair.second.is_alive ? "alive" : "dead") + "]" +
                 " missed: " + std::to_string(pair.second.missed_beats) + "\n";
    }
    
    return status;
}

void HeartbeatManager::heartbeatThreadFunc() {
    Logger::instance().info("Heartbeat thread started");
    
    while (running_) {
        try {
            std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms_));
            
            std::lock_guard<std::mutex> lock(mutex_);
            for (auto& pair : nodes_) {
                sendHeartbeat(pair.second);
            }
        } catch (const std::exception& e) {
            Logger::instance().error("Error in heartbeat thread: " + std::string(e.what()));
        }
    }
    
    Logger::instance().info("Heartbeat thread stopped");
}

void HeartbeatManager::checkThreadFunc() {
    Logger::instance().info("Health check thread started");
    
    while (running_) {
        try {
            std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms_));
            checkNodeHealth();
        } catch (const std::exception& e) {
            Logger::instance().error("Error in health check thread: " + std::string(e.what()));
        }
    }
    
    Logger::instance().info("Health check thread stopped");
}

void HeartbeatManager::sendHeartbeat(HeartbeatNodeInfo& node) {
    // TODO: 实际发送心跳的逻辑
    // 这里只是模拟更新心跳时间
    node.last_heartbeat_time = 
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    
    Logger::instance().debug("Sent heartbeat to node: " + node.id);
}

void HeartbeatManager::checkNodeHealth() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    for (auto& pair : nodes_) {
        auto& node = pair.second;
        if (node.is_alive) {
            // 将 timeout_ms_ 转换为 uint64_t 进行比较
            uint64_t timeout = static_cast<uint64_t>(timeout_ms_);
            if (now - node.last_heartbeat_time > timeout) {
                node.missed_beats++;
                if (node.missed_beats >= 3) {  // 连续3次超时标记为dead
                    node.is_alive = false;
                    Logger::instance().warning("Node " + node.id + " marked as dead");
                }
            } else {
                node.missed_beats = 0;
            }
        }
    }
}
