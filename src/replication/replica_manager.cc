#include "replica_manager.h"
#include "common/logger.h"
#include "common/protocol.h"
#include "core/kv_store.h"
#include "replication/replication_log.h"
#include "replication/master.h"
#include "replication/slave.h"

#include <chrono>
#include <thread>
#include <sstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

ReplicaManager::ReplicaManager(KVStore* store, Role role, const std::string& node_id)
    : role_(role)
    , node_id_(node_id)
    , store_(store)
    , running_(false)
    , replication_log_(std::make_unique<ReplicationLog>())
{
    Logger::info("ReplicaManager initialized with role: %s", 
                 (role == MASTER ? "MASTER" : "SLAVE"));
}

ReplicaManager::~ReplicaManager() {
    stop();
}

void ReplicaManager::addSlave(const std::string& host, int port) {
    if (role_ != MASTER) {
        Logger::error("Only master can add slaves");
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 检查是否已存在
    for (const auto& slave : slaves_) {
        if (slave.host == host && slave.port == port) {
            Logger::warn("Slave %s:%d already exists", host.c_str(), port);
            return;
        }
    }
    
    ReplicaInfo slave;
    slave.id = "slave-" + std::to_string(slaves_.size() + 1);
    slave.host = host;
    slave.port = port;
    slave.is_alive = true;
    slave.next_log_id = 1;  // 从第一条日志开始
    
    slaves_.push_back(slave);
    Logger::info("Added slave %s at %s:%d", slave.id.c_str(), host.c_str(), port);
}

void ReplicaManager::setMaster(const std::string& host, int port) {
    if (role_ != SLAVE) {
        Logger::error("Only slave can set master");
        return;
    }
    
    master_host_ = host;
    master_port_ = port;
    Logger::info("Set master to %s:%d", host.c_str(), port);
}

void ReplicaManager::start() {
    if (running_) {
        Logger::warn("ReplicaManager already running");
        return;
    }
    
    running_ = true;
    
    if (role_ == MASTER) {
        // 启动主节点复制线程
        replication_thread_ = std::thread(&ReplicaManager::masterReplicationThread, this);
        Logger::info("Master replication thread started");
    } else if (role_ == SLAVE && !master_host_.empty()) {
        // 启动从节点同步线程
        replication_thread_ = std::thread(&ReplicaManager::slaveSyncThread, this);
        Logger::info("Slave sync thread started");
    }
}

void ReplicaManager::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    
    if (replication_thread_.joinable()) {
        replication_thread_.join();
        Logger::info("Replication thread stopped");
    }
}

bool ReplicaManager::handleWrite(const std::string& key, const std::string& value, bool is_delete) {
    if (role_ != MASTER) {
        Logger::error("Only master can handle writes");
        return false;
    }
    
    // 1. 写入本地存储
    bool success = false;
    if (is_delete) {
        success = store_->deleteKey(key);
    } else {
        success = store_->put(key, value);
    }
    
    if (!success) {
        Logger::error("Failed to write to local store");
        return false;
    }
    
    // 2. 记录到复制日志
    LogType type = is_delete ? LogType::DELETE : LogType::PUT;
    uint64_t log_id = replication_log_->append(type, key, value);
    
    Logger::debug("Logged operation: %s %s (log_id: %lu)", 
                  (is_delete ? "DELETE" : "PUT"), key.c_str(), log_id);
    
    return true;
}

std::string ReplicaManager::getStatus() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::stringstream ss;
    ss << "Role: " << (role_ == MASTER ? "MASTER" : "SLAVE") << "\n";
    ss << "Node ID: " << node_id_ << "\n";
    ss << "Running: " << (running_ ? "true" : "false") << "\n";
    
    if (role_ == MASTER) {
        ss << "Slave count: " << slaves_.size() << "\n";
        for (const auto& slave : slaves_) {
            ss << "  " << slave.id << " [" << slave.host << ":" << slave.port 
               << "] alive=" << slave.is_alive 
               << " next_log=" << slave.next_log_id << "\n";
        }
    } else {
        ss << "Master: " << master_host_ << ":" << master_port_ << "\n";
    }
    
    ss << "Last log ID: " << replication_log_->getLastLogId();
    return ss.str();
}

void ReplicaManager::masterReplicationThread() {
    Logger::info("Master replication thread starting...");
    
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        // 获取最新的日志ID
        uint64_t last_log_id = replication_log_->getLastLogId();
        if (last_log_id == 0) {
            continue;  // 没有新日志
        }
        
        // 向每个从节点发送未复制的日志
        for (auto& slave : slaves_) {
            if (!slave.is_alive) {
                // 可以在这里实现重连逻辑
                continue;
            }
            
            // 获取需要发送的日志
            if (slave.next_log_id > last_log_id) {
                continue;  // 从节点已经同步到最新
            }
            
            int max_count = 10;  // 每次最多发送10条日志
            auto logs = replication_log_->getEntriesFrom(slave.next_log_id, max_count);
            
            if (logs.empty()) {
                continue;
            }
            
            // 发送日志到从节点
            if (sendLogToSlave(slave, logs)) {
                slave.next_log_id = logs.back().log_id + 1;
                Logger::debug("Sent logs %lu-%lu to slave %s", 
                            logs.front().log_id, logs.back().log_id, slave.id.c_str());
            } else {
                slave.is_alive = false;
                Logger::warn("Failed to send logs to slave %s, marking as dead", slave.id.c_str());
            }
        }
    }
    
    Logger::info("Master replication thread exiting...");
}

void ReplicaManager::slaveSyncThread() {
    Logger::info("Slave sync thread starting...");
    
    // 初始延迟，等待服务器启动
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    while (running_) {
        try {
            // 获取当前已应用的最后一个日志ID
            uint64_t start_id = replication_log_->getLastLogId() + 1;
            
            if (!fetchLogFromMaster(start_id)) {
                Logger::warn("Failed to fetch logs from master, retrying in 1s...");
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }
            
            // 应用新获取的日志到本地存储
            replication_log_->applyToStore(store_, start_id);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        } catch (const std::exception& e) {
            Logger::error("Error in slave sync thread: %s", e.what());
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }
    
    Logger::info("Slave sync thread exiting...");
}

bool ReplicaManager::sendLogToSlave(const ReplicaInfo& slave, 
                                   const std::vector<ReplicationLogEntry>& logs) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        Logger::error("Failed to create socket for slave %s", slave.id.c_str());
        return false;
    }
    
    // 设置超时
    struct timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    
    // 连接到从节点
    struct sockaddr_in slave_addr;
    memset(&slave_addr, 0, sizeof(slave_addr));
    slave_addr.sin_family = AF_INET;
    slave_addr.sin_port = htons(slave.port);
    
    if (inet_pton(AF_INET, slave.host.c_str(), &slave_addr.sin_addr) <= 0) {
        Logger::error("Invalid slave address: %s", slave.host.c_str());
        close(sockfd);
        return false;
    }
    
    if (connect(sockfd, (struct sockaddr*)&slave_addr, sizeof(slave_addr)) < 0) {
        close(sockfd);
        return false;
    }
    
    // 发送复制请求
    std::string request = "REPLICATE_LOG " + std::to_string(logs.size());
    for (const auto& log : logs) {
        request += " " + std::to_string(log.log_id) + ":" 
                   + (log.type == LogType::PUT ? "PUT:" : "DEL:")
                   + log.key;
        if (!log.value.empty()) {
            request += ":" + log.value;
        }
    }
    
    if (send(sockfd, request.c_str(), request.length(), 0) < 0) {
        Logger::error("Failed to send replication data to slave %s", slave.id.c_str());
        close(sockfd);
        return false;
    }
    
    // 等待响应
    char buffer[256];
    int bytes = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        std::string response(buffer);
        close(sockfd);
        
        if (response.find("OK") == 0) {
            return true;
        }
    }
    
    close(sockfd);
    return false;
}

bool ReplicaManager::fetchLogFromMaster(uint64_t start_id) {
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
    
    // 发送获取日志请求
    std::string request = "GET_LOG " + std::to_string(start_id);
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
    
    // 解析响应并添加到本地日志
    // 格式: OK log_count log1:log2:...
    if (response.find("OK ") == 0) {
        // 这里简化处理，实际需要解析日志并添加到replication_log_
        Logger::debug("Received %d bytes of log data from master", bytes);
        return true;
    }
    
    return false;
}
