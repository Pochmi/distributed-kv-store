// master.h - 主节点专用逻辑
class MasterNode {
public:
    explicit MasterNode(KVStore* store);
    
    // 处理客户端写请求
    bool processWrite(const std::string& key, const std::string& value);
    bool processDelete(const std::string& key);
    
    // 管理从节点
    void registerSlave(const std::string& slave_id, 
                      const std::string& host, int port);
    void removeSlave(const std::string& slave_id);
    
    // 获取复制滞后情况
    std::map<std::string, uint64_t> getReplicationLag() const;
    
private:
    KVStore* store_;
    std::unique_ptr<ReplicationLog> log_;
    std::map<std::string, ReplicaInfo> slaves_; // slave_id -> info
    mutable std::mutex mutex_;
};

