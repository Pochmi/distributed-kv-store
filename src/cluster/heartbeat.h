#ifndef CLUSTER_HEARTBEAT_H
#define CLUSTER_HEARTBEAT_H

#include <string>
#include <functional>
#include <atomic>
#include <thread>
#include <map>
#include <mutex>

class HeartbeatManager {
public:
    using Callback = std::function<void(const std::string&, bool)>;
    
    HeartbeatManager(const std::string& node_id, int port);
    ~HeartbeatManager();
    
    // 启动心跳（发送和接收）
    void start(int interval_ms = 1000);
    void stop();
    
    // 注册收到心跳的回调
    void setCallback(Callback cb);
    
    // 记录收到的心跳
    void recordHeartbeat(const std::string& node_id);
    
    // 检查节点是否存活
    bool isAlive(const std::string& node_id);
    
    // 添加节点到监控列表
    void addNode(const std::string& node_id, const std::string& host, int port);
    
    // 获取所有故障节点
    std::vector<std::string> getFailedNodes();
    
    // 定期检查超时
    void checkTimeout();

    // 获取状态信息
    std::string getStatus() const;
    
private:
    void sendLoop();
    void receiveLoop();
    
    std::string node_id_;
    int heartbeat_port_;  // 心跳端口 = 服务端口 + 100
    int interval_ms_;
    int timeout_ms_;
    
    std::atomic<bool> running_;
    std::thread send_thread_;
    std::thread recv_thread_;
    
    struct NodeStatus {
        std::string host;
        int port;
        bool alive;
        long last_heartbeat_ms;
        int missed_count;
    };
    std::map<std::string, NodeStatus> nodes_;
    std::mutex mutex_;
    
    Callback callback_;
};

#endif