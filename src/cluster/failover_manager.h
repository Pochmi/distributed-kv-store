#ifndef CLUSTER_FAILOVER_MANAGER_H
#define CLUSTER_FAILOVER_MANAGER_H

#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include "heartbeat.h"

class FailoverManager {
public:
    using Callback = std::function<void(const std::string&, const std::string&)>;
    
    FailoverManager(HeartbeatManager* heartbeat);
    ~FailoverManager();
    
    // 设置当前主节点
    void setMaster(const std::string& node_id);
    
    // 获取当前主节点
    std::string getMaster() const;
    
    // 添加从节点（候选）
    void addSlave(const std::string& node_id);
    
    // 检查并执行故障切换
    void checkAndFailover();
    
    // 手动切换
    bool manualFailover(const std::string& new_master);
    
    // 注册切换回调
    void setFailoverCallback(Callback cb);
    
private:
    HeartbeatManager* heartbeat_;
    std::string current_master_;
    std::vector<std::string> slaves_;
    Callback callback_;
    mutable std::mutex mutex_;
};

#endif