#include "replication_log.h"
#include "common/logger.h"
#include "core/kv_store.h"

#include <sstream>
#include <algorithm>

ReplicationLog::ReplicationLog() : next_log_id_(1) {
    entries_.reserve(1000);  // 预分配空间
    Logger::debug("ReplicationLog initialized");
}

uint64_t ReplicationLog::append(LogType type, const std::string& key, 
                               const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    ReplicationLogEntry entry;
    entry.log_id = next_log_id_++;
    entry.term = 1;  // 简化版，固定为1
    entry.type = type;
    entry.key = key;
    entry.value = value;
    entry.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    entries_.push_back(entry);
    
    // 限制日志大小（可选）
    if (entries_.size() > 10000) {
        entries_.erase(entries_.begin(), entries_.begin() + 1000);
        Logger::info("ReplicationLog trimmed to %zu entries", entries_.size());
    }
    
    return entry.log_id;
}

std::vector<ReplicationLogEntry> ReplicationLog::getEntriesFrom(uint64_t start_id, int max_count) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ReplicationLogEntry> result;
    
    // 找到起始位置
    auto it = std::lower_bound(entries_.begin(), entries_.end(), start_id,
        [](const ReplicationLogEntry& entry, uint64_t id) {
            return entry.log_id < id;
        });
    
    if (it == entries_.end()) {
        return result;  // 没有找到
    }
    
    // 收集最多max_count条日志
    int count = 0;
    for (; it != entries_.end() && count < max_count; ++it, ++count) {
        result.push_back(*it);
    }
    
    return result;
}

uint64_t ReplicationLog::getLastLogId() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (entries_.empty()) {
        return 0;
    }
    return entries_.back().log_id;
}

bool ReplicationLog::applyToStore(KVStore* store, uint64_t start_id) {
    if (!store) {
        Logger::error("Cannot apply logs: store is null");
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 找到起始位置
    auto it = std::lower_bound(entries_.begin(), entries_.end(), start_id,
        [](const ReplicationLogEntry& entry, uint64_t id) {
            return entry.log_id < id;
        });
    
    if (it == entries_.end()) {
        return true;  // 没有需要应用的日志
    }
    
    int applied_count = 0;
    for (; it != entries_.end(); ++it) {
        bool success = false;
        
        if (it->type == LogType::PUT) {
            success = store->put(it->key, it->value);
            if (success) {
                Logger::debug("Applied PUT log %lu: %s = %s", 
                            it->log_id, it->key.c_str(), it->value.c_str());
            }
        } else if (it->type == LogType::DELETE) {
            success = store->deleteKey(it->key);
            if (success) {
                Logger::debug("Applied DELETE log %lu: %s", 
                            it->log_id, it->key.c_str());
            }
        }
        
        if (!success) {
            Logger::error("Failed to apply log %lu: %s %s", 
                         it->log_id, 
                         (it->type == LogType::PUT ? "PUT" : "DELETE"),
                         it->key.c_str());
            return false;
        }
        
        applied_count++;
    }
    
    Logger::info("Applied %d logs to store starting from id %lu", applied_count, start_id);
    return true;
}

// 工具函数：将日志条目转为字符串
std::string ReplicationLog::entryToString(const ReplicationLogEntry& entry) {
    std::stringstream ss;
    ss << entry.log_id << ":"
       << entry.term << ":"
       << (entry.type == LogType::PUT ? "PUT" : "DEL") << ":"
       << entry.key << ":"
       << entry.value << ":"
       << entry.timestamp;
    return ss.str();
}

// 工具函数：从字符串解析日志条目
ReplicationLogEntry ReplicationLog::stringToEntry(const std::string& str) {
    ReplicationLogEntry entry;
    std::stringstream ss(str);
    std::string token;
    std::vector<std::string> tokens;
    
    while (std::getline(ss, token, ':')) {
        tokens.push_back(token);
    }
    
    if (tokens.size() >= 6) {
        entry.log_id = std::stoull(tokens[0]);
        entry.term = std::stoull(tokens[1]);
        entry.type = (tokens[2] == "PUT") ? LogType::PUT : LogType::DELETE;
        entry.key = tokens[3];
        entry.value = tokens[4];
        entry.timestamp = std::stoull(tokens[5]);
    }
    
    return entry;
}
