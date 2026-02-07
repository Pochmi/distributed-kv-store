#ifndef FAILOVER_MANAGER_H
#define FAILOVER_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <thread>
#include <atomic>

struct ClusterNode {
    std::string id;
    std::string host;
    int port;
    std::string role;  // "master", "slave", "candidate"
    int priority;
    bool is_alive;
};

class FailoverManager {
public:
    FailoverManager(const std::string& current_node_id);
    ~FailoverManager();
    
    void addNode(const ClusterNode& node);
    void removeNode(const std::string& node_id);
    
    void startMonitoring();
    void stopMonitoring();
    
    bool promoteSlaveToMaster(const std::string& slave_id);
    bool demoteMasterToSlave(const std::string& master_id);
    
    std::string getMasterId() const;
    std::vector<std::string> getSlaveIds() const;
    
    std::string getClusterStatus() const;
    
private:
    void monitorThreadFunc();
    void detectMasterFailure();
    ClusterNode selectNewMaster();
    bool initiateElection();
    
    std::string current_node_id_;
    std::map<std::string, ClusterNode> cluster_nodes_;
    mutable std::mutex mutex_;
    
    std::thread monitor_thread_;
    std::atomic<bool> monitoring_;
};

#endif // FAILOVER_MANAGER_H
