#ifndef CLUSTER_FAILURE_DETECTOR_H
#define CLUSTER_FAILURE_DETECTOR_H

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <functional>
#include <chrono>

class FailureDetector {
public:
    using Callback = std::function<void(const std::string&, bool)>;
    
    FailureDetector();
    ~FailureDetector() = default;
    
    // 记录收到的心跳
    void recordHeartbeat(const std::string& node_id);
    
    // 检查节点是否存活
    bool isAlive(const std::string& node_id) const;
    
    // 添加节点到监控列表
    void addNode(const std::string& node_id);
    
    // 移除节点
    void removeNode(const std::string& node_id);
    
    // 获取所有故障节点
    std::vector<std::string> getFailedNodes() const;
    
    // 获取所有存活节点
    std::vector<std::string> getAliveNodes() const;
    
    // 检查超时（定期调用）
    void checkTimeout(int timeout_ms = 3000, int max_missed = 3);
    
    // 设置回调（节点状态变化时调用）
    void setCallback(Callback cb);
    
    // 获取节点状态信息
    std::string getStatus() const;
    
private:
    struct NodeInfo {
        bool alive;
        long last_heartbeat_ms;
        int missed_count;
    };
    
    std::map<std::string, NodeInfo> nodes_;
    mutable std::mutex mutex_;
    Callback callback_;
    
    long getCurrentTimeMs() const;
};

#endif