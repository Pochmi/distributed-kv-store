#include "replication_log.h"
#include "../core/kv_store.h"
#include <chrono>

ReplicationLog::ReplicationLog() : next_log_id_(1) {}

uint64_t ReplicationLog::append(LogType type, const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    ReplicationLogEntry entry;
    entry.log_id = next_log_id_++;
    entry.term = 0;
    entry.type = type;
    entry.key = key;
    entry.value = value;
    entry.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    entries_.push_back(entry);
    return entry.log_id;
}

std::vector<ReplicationLogEntry> ReplicationLog::getEntriesFrom(uint64_t start_id, int max_count) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ReplicationLogEntry> result;
    
    for (const auto& entry : entries_) {
        if (entry.log_id >= start_id) {
            result.push_back(entry);
            if (result.size() >= (size_t)max_count) break;
        }
    }
    return result;
}

uint64_t ReplicationLog::getLastLogId() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (entries_.empty()) return 0;
    return entries_.back().log_id;
}

bool ReplicationLog::applyToStore(KVStore* store, uint64_t start_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& entry : entries_) {
        if (entry.log_id >= start_id) {
            if (entry.type == LogType::PUT) {
                store->Put(entry.key, entry.value);
            } else {
                store->Delete(entry.key);
            }
        }
    }
    return true;
}