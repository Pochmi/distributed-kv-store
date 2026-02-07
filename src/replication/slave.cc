#include "slave.h"
#include "common/logger.h"
#include "core/kv_store.h"
#include "replication/replication_log.h"

#include <thread>
#include <chrono>
#include <sstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

SlaveNode::SlaveNode(KVStore* store, const std::string& master_host, int master_port)
    : store_(store)
    , master_host_(master_host)
    , master_port_(master_port)
    , syncing_(false)
    , last_applied_log_id_(0)
    , last_sync_time_(0) {
    Logger::info("SlaveNode initialized for master %s:%d", 
                 master_host.c_str(), master_port);
}

SlaveNode::~SlaveNode() {
    disconnectFromMaster();
}

bool SlaveNode::connectToMaster() {
    if (syncing_) {
        Logger::warn("Already connected to master");
        return true;
    }
    
    Logger::info("Connecting to master at %s:%d...", master_host_.c_str(), master_port_);
    
    // 创建socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        Logger::error("Failed to create socket");
        return false;
    }
    
    // 设置超时
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    
    // 连接主节点
    struct sockaddr_in master_addr;
    memset(&master_addr, 0, sizeof(master_addr));
    master_addr.sin_family = AF_INET;
    master_addr.sin_port = htons(master_port_);
    
    if (inet_pton(AF_INET, master_host_.c_str(), &master_addr.sin_addr) <= 0) {
        Logger::error("Invalid master address: %s", master_host_.c_str());
        close(sockfd);
        return false;
    }
    
    if (connect(sockfd, (struct sockaddr*)&master_addr, sizeof(master_addr)) < 0) {
        Logger::error("Failed to connect to master");
        close(sockfd);
        return false;
    }
    
    close(sockfd);
    
    // 启动同步线程
    syncing_ = true;
    sync_thread_ = std::thread(&SlaveNode::syncThreadFunc, this);
    
    Logger::info("Successfully connected to master");
    return true;
}

void SlaveNode::disconnectFromMaster() {
    if (!syncing_) {
        return;
    }
    
    Logger::info("Disconnecting from master...");
    
    syncing_ = false;
    if (sync_thread_.joinable()) {
        sync_thread_.join();
    }
    
    Logger::info("Disconnected from master");
}

bool SlaveNode::applyLogEntry(const ReplicationLogEntry& entry) {
    if (entry.log_id <= last_applied_log_id_) {
        Logger::debug("Log %lu already applied, skipping", entry.log_id);
        return true;
    }
    
    bool success = false;
    
    if (entry.type == LogType::PUT) {
        success = store_->put(entry.key, entry.value);
        if (success) {
            Logger::debug("Applied PUT log %lu: %s = %s", 
                         entry.log_id, entry.key.c_str(), entry.value.c_str());
        } else {
            Logger::error("Failed to apply PUT log %lu: %s", 
                         entry.log_id, entry.key.c_str());
        }
    } else if (entry.type == LogType::DELETE) {
        success = store_->deleteKey(entry.key);
        if (success) {
            Logger::debug("Applied DELETE log %lu: %s", 
                         entry.log_id, entry.key.c_str());
        } else {
            Logger::error("Failed to apply DELETE log %lu: %s", 
                         entry.log_id, entry.key.c_str());
        }
    }
    
    if (success) {
        last_applied_log_id_ = entry.log_id;
        last_sync_time_ = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }
    
    return success;
}

bool SlaveNode::isSyncing() const {
    return syncing_;
}

uint64_t SlaveNode::getLastAppliedLogId() const {
    return last_applied_log_id_;
}

std::string SlaveNode::getStatus() const {
    std::stringstream ss;
    ss << "Slave Status:\n";
    ss << "  Master: " << master_host_ << ":" << master_port_ << "\n";
    ss << "  Syncing: " << (syncing_ ? "true" : "false") << "\n";
    ss << "  Last applied log ID: " << last_applied_log_id_ << "\n";
    
    if (last_sync_time_ > 0) {
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        ss << "  Time since last sync: " << (now - last_sync_time_) / 1000.0 << "s\n";
    }
    
    return ss.str();
}

void SlaveNode::syncThreadFunc() {
    Logger::info("Slave sync thread started");
    
    int retry_count = 0;
    const int max_retries = 10;
    
    while (syncing_ && retry_count < max_retries) {
        try {
            // 尝试从主节点获取日志
            if (fetchLogsFromMaster()) {
                retry_count = 0;  // 重置重试计数
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            } else {
                retry_count++;
                Logger::warn("Failed to fetch logs (attempt %d/%d), retrying in 1s...", 
                           retry_count, max_retries);
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        } catch (const std::exception& e) {
            retry_count++;
            Logger::error("Exception in sync thread: %s", e.what());
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }
    
    if (retry_count >= max_retries) {
        Logger::error("Max retries reached, stopping sync");
        syncing_ = false;
    }
    
    Logger::info("Slave sync thread stopped");
}

bool SlaveNode::fetchLogsFromMaster() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        return false;
    }
    
    // 设置超时
    struct timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    
    // 连接到主节点
    struct sockaddr_in master_addr;
    memset(&master_addr, 0, sizeof(master_addr));
    master_addr.sin_family = AF_INET;
    master_addr.sin_port = htons(master_port_);
    
    if (inet_pton(AF_INET, master_host_.c_str(), &master_addr.sin_addr) <= 0) {
        close(sockfd);
        return false;
    }
    
    if (connect(sockfd, (struct sockaddr*)&master_addr, sizeof(master_addr)) < 0) {
        close(sockfd);
        return false;
    }
    
    // 发送同步请求
    std::string request = "SYNC " + std::to_string(last_applied_log_id_ + 1);
    if (send(sockfd, request.c_str(), request.length(), 0) < 0) {
        close(sockfd);
        return false;
    }
    
    // 接收响应
    char buffer[4096];
    int bytes = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (bytes <= 0) {
        close(sockfd);
        return false;
    }
    
    buffer[bytes] = '\0';
    std::string response(buffer);
    close(sockfd);
    
    // 解析响应并应用日志
    // 简化版：假设响应格式为 "OK log_count log_data"
    if (response.find("OK ") == 0) {
        // 这里应该解析具体的日志数据
        Logger::debug("Received sync response: %s", response.substr(0, 50).c_str());
        
        // 模拟应用日志
        last_applied_log_id_++;
        last_sync_time_ = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        
        return true;
    }
    
    return false;
}
