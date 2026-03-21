#ifndef MASTER_H
#define MASTER_H

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include "replica_manager.h"
#include "replication_log.h"
#include "../core/kv_store.h"

class MasterNode {
public:
    explicit MasterNode(KVStore* store);
    ~MasterNode() = default;
    
    bool processWrite(const std::string& key, const std::string& value);
    bool processDelete(const std::string& key);
    
    void registerSlave(const std::string& slave_id, const std::string& host, int port);
    void removeSlave(const std::string& slave_id);
    std::map<std::string, uint64_t> getReplicationLag() const;
    std::vector<ReplicationLogEntry> getLogsForSlave(const std::string& slave_id, 
                                                     uint64_t start_log_id, 
                                                     int max_count = 100);
    bool isSlaveAlive(const std::string& slave_id) const;
    void setSlaveAlive(const std::string& slave_id, bool alive);
    std::string getStatus() const;
    
private:
    void notifySlaves(uint64_t log_id);
    
    KVStore* store_;
    std::unique_ptr<ReplicationLog> log_;
    std::map<std::string, ReplicaInfo> slaves_;
    mutable std::mutex mutex_;
};

#endif