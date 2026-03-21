#include "slave.h"
#include "../common/logger.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <chrono>
#include <thread>

SlaveNode::SlaveNode(KVStore* store, const std::string& master_host, int master_port)
    : store_(store)
    , master_host_(master_host)
    , master_port_(master_port)
    , socket_fd_(-1)
    , syncing_(false)
    , last_applied_log_id_(0) {
    Logger::instance().info("SlaveNode initialized for master " + master_host + ":" + 
                           std::to_string(master_port));
}

SlaveNode::~SlaveNode() {
    disconnectFromMaster();
}

bool SlaveNode::connectToMaster() {
    if (syncing_) {
        Logger::instance().warning("Already connected to master");
        return true;
    }
    
    Logger::instance().info("Connecting to master at " + master_host_ + ":" + 
                           std::to_string(master_port_) + "...");
    
    socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd_ < 0) {
        Logger::instance().error("Failed to create socket");
        return false;
    }
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(master_port_);
    
    if (inet_pton(AF_INET, master_host_.c_str(), &server_addr.sin_addr) <= 0) {
        Logger::instance().error("Invalid master address: " + master_host_);
        close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }
    
    if (::connect(socket_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        Logger::instance().error("Failed to connect to master: " + std::string(strerror(errno)));
        close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }
    
    Logger::instance().info("Successfully connected to master!");
    syncing_ = true;
    return true;
}

void SlaveNode::disconnectFromMaster() {
    if (!syncing_) return;
    
    Logger::instance().info("Disconnecting from master...");
    syncing_ = false;
    
    if (sync_thread_.joinable()) sync_thread_.join();
    if (socket_fd_ != -1) {
        close(socket_fd_);
        socket_fd_ = -1;
    }
    
    Logger::instance().info("Disconnected from master");
}

bool SlaveNode::applyLogEntry(const ReplicationLogEntry& entry) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (entry.log_id <= last_applied_log_id_) {
        Logger::instance().debug("Log " + std::to_string(entry.log_id) + " already applied");
        return true;
    }
    
    bool success = false;
    if (entry.type == LogType::PUT) {
        Status status = store_->Put(entry.key, entry.value);
        success = status.ok();
        if (success) {
            Logger::instance().debug("Applied PUT log " + std::to_string(entry.log_id) + 
                                    ": " + entry.key + " = " + entry.value);
        }
    } else if (entry.type == LogType::DELETE) {
        Status status = store_->Delete(entry.key);
        success = status.ok();
        if (success) {
            Logger::instance().debug("Applied DELETE log " + std::to_string(entry.log_id) + 
                                    ": " + entry.key);
        }
    }
    
    if (success) last_applied_log_id_ = entry.log_id;
    return success;
}

bool SlaveNode::isSyncing() const { return syncing_; }

uint64_t SlaveNode::getLastAppliedLogId() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return last_applied_log_id_;
}

std::string SlaveNode::getStatus() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string status = "Slave Node Status:\n";
    status += "  Master: " + master_host_ + ":" + std::to_string(master_port_) + "\n";
    status += "  Syncing: " + std::string(syncing_ ? "true" : "false") + "\n";
    status += "  Last applied log ID: " + std::to_string(last_applied_log_id_) + "\n";
    status += "  Socket FD: " + std::to_string(socket_fd_) + "\n";
    return status;
}

void SlaveNode::syncThreadFunc() {
    Logger::instance().info("Slave sync thread started");
    
    if (!connectToMaster()) {
        Logger::instance().error("Failed to connect to master");
        return;
    }
    
    while (syncing_) {
        if (fetchLogsFromMaster()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } else {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    
    Logger::instance().info("Slave sync thread stopped");
}

bool SlaveNode::fetchLogsFromMaster() {
    // 发送请求
    std::string request = "SYNC_REQ:" + std::to_string(last_applied_log_id_) + "\n";
    ssize_t sent = send(socket_fd_, request.c_str(), request.length(), 0);
    if (sent <= 0) return false;
    
    // 接收响应
    char buffer[4096];
    ssize_t received = recv(socket_fd_, buffer, sizeof(buffer) - 1, 0);
    if (received <= 0) return false;
    
    buffer[received] = '\0';
    std::string response(buffer);
    
    // 解析响应
    if (response.substr(0, 3) == "LOG") {
        size_t pos1 = response.find(' ', 4);
        size_t pos2 = response.find(' ', pos1 + 1);
        size_t pos3 = response.find(' ', pos2 + 1);
        
        if (pos1 != std::string::npos && pos2 != std::string::npos) {
            try {
                uint64_t log_id = std::stoull(response.substr(4, pos1 - 4));
                std::string type = response.substr(pos1 + 1, pos2 - pos1 - 1);
                std::string key = response.substr(pos2 + 1, pos3 - pos2 - 1);
                std::string value = response.substr(pos3 + 1);
                
                ReplicationLogEntry entry;
                entry.log_id = log_id;
                entry.type = (type == "PUT") ? LogType::PUT : LogType::DELETE;
                entry.key = key;
                entry.value = value;
                
                applyLogEntry(entry);
                Logger::instance().info("Applied log " + std::to_string(log_id) + " from master: " + key);
                return true;
            } catch (const std::exception& e) {
                Logger::instance().error("Failed to parse log: " + std::string(e.what()));
            }
        }
    }
    
    return false;
}