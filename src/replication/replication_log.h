#ifndef REPLICATION_LOG_H
#define REPLICATION_LOG_H

#include <string>
#include <vector>
#include <cstdint>
#include <mutex>

enum class LogType {
    PUT,
    DELETE
};

struct ReplicationLogEntry {
    uint64_t log_id;
    uint64_t term;
    LogType type;
    std::string key;
    std::string value;
    uint64_t timestamp;
};

class ReplicationLog {
public:
    ReplicationLog();
    
    uint64_t append(LogType type, const std::string& key, const std::string& value = "");
    std::vector<ReplicationLogEntry> getEntriesFrom(uint64_t start_id, int max_count);
    uint64_t getLastLogId() const;
    bool applyToStore(class KVStore* store, uint64_t start_id = 0);
    
private:
    std::vector<ReplicationLogEntry> entries_;
    mutable std::mutex mutex_;
    uint64_t next_log_id_;
};

#endif