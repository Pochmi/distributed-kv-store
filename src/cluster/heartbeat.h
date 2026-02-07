#ifndef HEARTBEAT_H
#define HEARTBEAT_H

#include <string>
#include <map>
#include <mutex>
#include <thread>
#include <atomic>
#include <vector>

struct NodeInfo {
    std::string id;
    std::string host;
    int port;
    bool is_alive;
    uint64_t last_heartbeat_time;
    int missed_beats;
};

struct HeartbeatStatus {
    int total_nodes;
    int alive_nodes;
    int dead_nodes;
};

class HeartbeatManager {
public:
    HeartbeatManager(const std::string& node_id, 
                    int interval_ms = 1000, 
                    int timeout_ms = 3000);
    ~HeartbeatManager();
    
    void addNode(const std::string& node_id, 
                const std::string& host, int port);
    void removeNode(const std::string& node_id);
    
    void start();
    void stop();
    
    HeartbeatStatus getNodeStatus() const;
    std::vector<std::string> getDeadNodes() const;
    std::string getStatus() const;
    
private:
    void heartbeatThreadFunc();
    void checkThreadFunc();
    void sendHeartbeat(NodeInfo& node);
    void checkNodeHealth();
    
    std::string node_id_;
    int interval_ms_;
    int timeout_ms_;
    
    std::map<std::string, NodeInfo> nodes_;
    mutable std::mutex mutex_;
    
    std::thread heartbeat_thread_;
    std::thread check_thread_;
    std::atomic<bool> running_;
};

#endif // HEARTBEAT_H
