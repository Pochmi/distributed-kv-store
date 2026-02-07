// slave.h - 从节点专用逻辑
class SlaveNode {
public:
    SlaveNode(KVStore* store, const std::string& master_host, int master_port);
    
    // 连接到主节点并开始同步
    bool connectToMaster();
    void disconnectFromMaster();
    
    // 处理来自主节点的日志
    bool applyLogEntry(const ReplicationLogEntry& entry);
    
    // 状态查询
    bool isSyncing() const;
    uint64_t getLastAppliedLogId() const;
    
private:
    KVStore* store_;
    std::string master_host_;
    int master_port_;
    std::thread sync_thread_;
    std::atomic<bool> syncing_;
    uint64_t last_applied_log_id_;
};
