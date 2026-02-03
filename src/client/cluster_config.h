#ifndef CLIENT_CLUSTER_CONFIG_H
#define CLIENT_CLUSTER_CONFIG_H

#include <string>
#include <vector>

struct NodeInfo {
    std::string id;
    std::string host;
    int port;
    std::string role;
    bool is_available{true};
    
    std::string address() const {
        return host + ":" + std::to_string(port);
    }
};

class ClusterConfig {
public:
    static ClusterConfig& getInstance();
    void initDefaultConfig();
    NodeInfo getNodeByKey(const std::string& key);
    std::vector<NodeInfo> getAllNodes() const { return nodes_; }
    
private:
    ClusterConfig() = default;
    std::vector<NodeInfo> nodes_;
};
#endif
