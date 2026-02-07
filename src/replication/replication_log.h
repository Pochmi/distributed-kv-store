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
    uint64_t term;  // 用于简化版的一致性
    LogType type;
    std::string key;
    std::string value;
    uint64_t timestamp;
};

class ReplicationLog {
public:
    ReplicationLog();
    
    // 添加日志条目
    uint64_t append(LogType type, const std::string& key, 
                   const std::string& value = "");
    
    // 获取从指定位置开始的日志
    std::vector<ReplicationLogEntry> getEntriesFrom(uint64_t start_id, int max_count);
    
    // 获取最后一条日志ID
    uint64_t getLastLogId() const;
    
    // 应用到存储引擎
    bool applyToStore(class KVStore* store, uint64_t start_id = 0);
    
private:
    std::vector<ReplicationLogEntry> entries_;
    mutable std::mutex mutex_;
    uint64_t next_log_id_;
};

#endif // REPLICATION_LOG_H
