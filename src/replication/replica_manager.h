#ifndef REPLICA_MANAGER_H
#define REPLICA_MANAGER_H

#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include "replication_log.h"

class KVStore;

struct ReplicaInfo {
    std::string id;
    std::string host;
    int port;
    bool is_alive;
    uint64_t next_log_id;
};

class ReplicaManager {
public:
    enum Role { MASTER, SLAVE };
    
    ReplicaManager(KVStore* store, Role role, const std::string& node_id);
    ~ReplicaManager();
    Role role() const { return role_; }
    void addSlave(const std::string& host, int port);
    void setMaster(const std::string& host, int port);
    void start();
    void stop();
    bool handleWrite(const std::string& key, const std::string& value, bool is_delete = false);
    std::string getStatus() const;
    
private:
    void masterReplicationThread();
    void slaveSyncThread();
    bool sendLogToSlave(const ReplicaInfo& slave, const std::vector<ReplicationLogEntry>& logs);
    bool fetchLogFromMaster(uint64_t start_id);
    bool connectToMaster();
    
    Role role_;
    std::string node_id_;
    KVStore* store_;
    std::unique_ptr<ReplicationLog> replication_log_;
    
    std::vector<ReplicaInfo> slaves_;
    std::string master_host_;
    int master_port_;
    
    std::thread replication_thread_;
    std::atomic<bool> running_;
    mutable std::mutex mutex_;
    int slave_socket_{-1};
};

#endif