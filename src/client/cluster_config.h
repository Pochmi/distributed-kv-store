#ifndef CLUSTER_CONFIG_H
#define CLUSTER_CONFIG_H

#include <string>
#include <vector>
#include <mutex>
#include <functional>

struct NodeInfo {
    std::string id;
    std::string host;
    int port;
    std::string role;
    bool is_healthy;
    int shard_id;
    
    NodeInfo() : port(0), is_healthy(true), shard_id(-1) {}
    NodeInfo(const std::string& i, const std::string& h, int p, 
             const std::string& r = "master", bool healthy = true, int sid = -1)
        : id(i), host(h), port(p), role(r), is_healthy(healthy), shard_id(sid) {}
    
    std::string address() const {
        return host + ":" + std::to_string(port);
    }
};

class ClusterConfig {
public:
    static ClusterConfig& getInstance();
    
    bool loadFromFile(const std::string& config_file);
    bool loadFromJson(const std::string& json_str);
    NodeInfo getNodeByKey(const std::string& key);
    std::vector<NodeInfo> getAllNodes() const;
    std::vector<NodeInfo> getAvailableMasters() const;
    size_t getNodeCount() const;
    void markNodeUnhealthy(const std::string& node_id);
    void markNodeHealthy(const std::string& node_id);
    void setRefreshCallback(std::function<void()> callback) { refresh_callback_ = callback; }
    
private:
    ClusterConfig();
    void initDefaultConfig();
    bool parseJsonConfig(const std::string& json_str);
    uint32_t hash(const std::string& key) const;
    
    std::vector<NodeInfo> nodes_;
    bool config_loaded_ = false;
    std::string config_file_;
    mutable std::mutex mutex_;
    std::function<void()> refresh_callback_;
};

#endif
