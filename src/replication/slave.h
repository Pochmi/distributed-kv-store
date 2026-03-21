#ifndef SLAVE_H
#define SLAVE_H

#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include "replication_log.h"
#include "../core/kv_store.h"

class SlaveNode {
public:
    SlaveNode(KVStore* store, const std::string& master_host, int master_port);
    ~SlaveNode();
    
    bool connectToMaster();
    void disconnectFromMaster();
    bool applyLogEntry(const ReplicationLogEntry& entry);
    bool isSyncing() const;
    uint64_t getLastAppliedLogId() const;
    std::string getStatus() const;
    
private:
    void syncThreadFunc();
    bool fetchLogsFromMaster();
    
    KVStore* store_;
    std::string master_host_;
    int master_port_;
    int socket_fd_;
    
    std::thread sync_thread_;
    std::atomic<bool> syncing_;
    uint64_t last_applied_log_id_;
    mutable std::mutex mutex_;
};

#endif