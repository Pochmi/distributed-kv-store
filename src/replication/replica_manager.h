#ifndef REPLICA_MANAGER_H
#define REPLICA_MANAGER_H

#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>

class KVStore;
class ReplicationLog;

struct ReplicaInfo {
    std::string id;
    std::string host;
    int port;
    bool is_alive;
    uint64_t next_log_id;  // 需要复制的下一个日志ID
};

class ReplicaManager {
public:
    enum Role {
        MASTER,
        SLAVE
    };
    
    ReplicaManager(KVStore* store, Role role, 
                  const std::string& node_id);
    ~ReplicaManager();
    
    // 主节点：添加从节点
    void addSlave(const std::string& host, int port);
    
    // 从节点：设置主节点
    void setMaster(const std::string& host, int port);
    
    // 启动复制
    void start();
    
    // 停止复制
    void stop();
    
    // 处理写操作（主节点调用）
    bool handleWrite(const std::string& key, const std::string& value, bool is_delete = false);
    
    // 获取复制状态
    std::string getStatus() const;
    
private:
    // 主节点：复制线程
    void masterReplicationThread();
    
    // 从节点：同步线程
    void slaveSyncThread();
    
    // 发送日志到从节点
    bool sendLogToSlave(const ReplicaInfo& slave, 
                       const std::vector<ReplicationLogEntry>& logs);
    
    // 从主节点拉取日志
    bool fetchLogFromMaster(uint64_t start_id);
    
    Role role_;
    std::string node_id_;
    KVStore* store_;
    std::unique_ptr<ReplicationLog> replication_log_;
    
    // 主节点的从节点列表
    std::vector<ReplicaInfo> slaves_;
    
    // 从节点的主节点信息
    std::string master_host_;
    int master_port_;
    
    // 线程控制
    std::thread replication_thread_;
    std::atomic<bool> running_;
    mutable std::mutex mutex_;
};

#endif // REPLICA_MANAGER_H
