#include "master.h"
#include "../common/logger.h"
#include <sstream>

MasterNode::MasterNode(KVStore* store) : store_(store), log_(std::make_unique<ReplicationLog>()) {
    Logger::instance().info("MasterNode initialized");
}

bool MasterNode::processWrite(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    Status status = store_->Put(key, value);
    if (!status.ok()) return false;
    
    uint64_t log_id = log_->append(LogType::PUT, key, value);
    Logger::instance().debug("Master logged PUT: " + key + " (log_id: " + std::to_string(log_id) + ")");
    notifySlaves(log_id);
    return true;
}

bool MasterNode::processDelete(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    Status status = store_->Delete(key);
    if (!status.ok()) return false;
    
    uint64_t log_id = log_->append(LogType::DELETE, key, "");
    Logger::instance().debug("Master logged DELETE: " + key + " (log_id: " + std::to_string(log_id) + ")");
    notifySlaves(log_id);
    return true;
}

void MasterNode::registerSlave(const std::string& slave_id, const std::string& host, int port) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (slaves_.find(slave_id) != slaves_.end()) {
        Logger::instance().warning("Slave " + slave_id + " already registered");
        return;
    }
    
    ReplicaInfo info;
    info.id = slave_id;
    info.host = host;
    info.port = port;
    info.is_alive = true;
    info.next_log_id = log_->getLastLogId() + 1;
    slaves_[slave_id] = info;
    Logger::instance().info("Registered slave " + slave_id + " at " + host + ":" + std::to_string(port));
}

void MasterNode::removeSlave(const std::string& slave_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    slaves_.erase(slave_id);
    Logger::instance().info("Removed slave " + slave_id);
}

std::map<std::string, uint64_t> MasterNode::getReplicationLag() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::map<std::string, uint64_t> lag;
    uint64_t last_id = log_->getLastLogId();
    for (const auto& kv : slaves_) {
        lag[kv.first] = last_id - kv.second.next_log_id + 1;
    }
    return lag;
}

std::vector<ReplicationLogEntry> MasterNode::getLogsForSlave(const std::string& slave_id,
                                                            uint64_t start_log_id,
                                                            int max_count) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = slaves_.find(slave_id);
    if (it == slaves_.end()) return {};
    return log_->getEntriesFrom(start_log_id, max_count);
}

bool MasterNode::isSlaveAlive(const std::string& slave_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = slaves_.find(slave_id);
    if (it != slaves_.end()) return it->second.is_alive;
    return false;
}

void MasterNode::setSlaveAlive(const std::string& slave_id, bool alive) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = slaves_.find(slave_id);
    if (it != slaves_.end()) {
        it->second.is_alive = alive;
        Logger::instance().info("Set slave " + slave_id + " alive: " + (alive ? "true" : "false"));
    }
}

std::string MasterNode::getStatus() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::stringstream ss;
    ss << "Master Node Status:\n";
    ss << "  Total slaves: " << slaves_.size() << "\n";
    ss << "  Last log ID: " << log_->getLastLogId() << "\n";
    for (const auto& kv : slaves_) {
        ss << "  Slave " << kv.first << ": " << kv.second.host << ":" << kv.second.port
           << " [" << (kv.second.is_alive ? "alive" : "dead") << "]"
           << " next_log: " << kv.second.next_log_id << "\n";
    }
    return ss.str();
}

void MasterNode::notifySlaves(uint64_t log_id) {
    // TODO: 实现真正的通知
    Logger::instance().debug("Notifying slaves about log " + std::to_string(log_id));
}