#include "master.h"
#include "common/logger.h"
#include "core/kv_store.h"
#include "replication/replication_log.h"

#include <thread>
#include <chrono>
#include <sstream>

MasterNode::MasterNode(KVStore* store)
    : store_(store)
    , log_(std::make_unique<ReplicationLog>())
    , last_applied_log_id_(0) {
    Logger::info("MasterNode initialized");
}

bool MasterNode::processWrite(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 1. 写入本地存储
    if (!store_->put(key, value)) {
        Logger::error("Failed to write to local store: %s", key.c_str());
        return false;
    }
    
    // 2. 记录到复制日志
    uint64_t log_id = log_->append(LogType::PUT, key, value);
    Logger::debug("Master logged PUT operation: %s = %s (log_id: %lu)", 
                  key.c_str(), value.c_str(), log_id);
    
    // 3. 异步通知从节点（在实际实现中，这里会触发复制）
    notifySlaves(log_id);
    
    return true;
}

bool MasterNode::processDelete(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 1. 从本地存储删除
    if (!store_->deleteKey(key)) {
        Logger::error("Failed to delete from local store: %s", key.c_str());
        return false;
    }
    
    // 2. 记录到复制日志
    uint64_t log_id = log_->append(LogType::DELETE, key, "");
    Logger::debug("Master logged DELETE operation: %s (log_id: %lu)", 
                  key.c_str(), log_id);
    
    // 3. 异步通知从节点
    notifySlaves(log_id);
    
    return true;
}

void MasterNode::registerSlave(const std::string& slave_id, 
                              const std::string& host, int port) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 检查是否已存在
    if (slaves_.find(slave_id) != slaves_.end()) {
        Logger::warn("Slave %s already registered", slave_id.c_str());
        return;
    }
    
    ReplicaInfo slave_info;
    slave_info.id = slave_id;
    slave_info.host = host;
    slave_info.port = port;
    slave_info.is_alive = true;
    slave_info.next_log_id = log_->getLastLogId() + 1;
    
    slaves_[slave_id] = slave_info;
    Logger::info("Registered slave %s at %s:%d", slave_id.c_str(), host.c_str(), port);
}

void MasterNode::removeSlave(const std::string& slave_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = slaves_.find(slave_id);
    if (it != slaves_.end()) {
        Logger::info("Removed slave %s", slave_id.c_str());
        slaves_.erase(it);
    } else {
        Logger::warn("Slave %s not found", slave_id.c_str());
    }
}

std::map<std::string, uint64_t> MasterNode::getReplicationLag() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::map<std::string, uint64_t> lag_map;
    
    uint64_t last_log_id = log_->getLastLogId();
    
    for (const auto& kv : slaves_) {
        const auto& slave = kv.second;
        if (slave.is_alive && slave.next_log_id <= last_log_id) {
            lag_map[slave.id] = last_log_id - slave.next_log_id + 1;
        } else if (!slave.is_alive) {
            lag_map[slave.id] = UINT64_MAX;  // 表示从节点已死亡
        } else {
            lag_map[slave.id] = 0;  // 已同步到最新
        }
    }
    
    return lag_map;
}

std::vector<ReplicationLogEntry> MasterNode::getLogsForSlave(const std::string& slave_id, 
                                                           uint64_t start_id, 
                                                           int max_count) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = slaves_.find(slave_id);
    if (it == slaves_.end()) {
        Logger::warn("Slave %s not found", slave_id.c_str());
        return {};
    }
    
    // 获取日志条目
    auto logs = log_->getEntriesFrom(start_id, max_count);
    
    // 更新从节点的下一个日志ID
    if (!logs.empty()) {
        it->second.next_log_id = logs.back().log_id + 1;
        it->second.is_alive = true;  // 标记为活跃
    }
    
    return logs;
}

bool MasterNode::isSlaveAlive(const std::string& slave_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = slaves_.find(slave_id);
    if (it != slaves_.end()) {
        return it->second.is_alive;
    }
    
    return false;
}

void MasterNode::setSlaveAlive(const std::string& slave_id, bool alive) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = slaves_.find(slave_id);
    if (it != slaves_.end()) {
        it->second.is_alive = alive;
        Logger::info("Set slave %s alive status to %s", 
                    slave_id.c_str(), alive ? "true" : "false");
    }
}

std::string MasterNode::getStatus() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::stringstream ss;
    ss << "Master Status:\n";
    ss << "  Total slaves: " << slaves_.size() << "\n";
    ss << "  Last log ID: " << log_->getLastLogId() << "\n";
    
    int alive_count = 0;
    for (const auto& kv : slaves_) {
        if (kv.second.is_alive) alive_count++;
    }
    ss << "  Alive slaves: " << alive_count << "\n";
    
    for (const auto& kv : slaves_) {
        const auto& slave = kv.second;
        ss << "  - " << slave.id << " [" << slave.host << ":" << slave.port 
           << "] alive=" << (slave.is_alive ? "true" : "false")
           << " next_log=" << slave.next_log_id << "\n";
    }
    
    return ss.str();
}

void MasterNode::notifySlaves(uint64_t log_id) {
    // 在实际实现中，这里会触发异步通知从节点
    // 简化版：只记录日志
    Logger::debug("Notifying slaves about new log %lu", log_id);
    
    // 可以在这里启动异步任务来复制日志
    // std::thread([this, log_id]() {
    //     replicateToSlaves(log_id);
    // }).detach();
}
