#ifndef CLUSTER_CONFIG_H
#define CLUSTER_CONFIG_H

#include <string>
#include <vector>
#include <memory>

struct NodeInfo {
    std::string id;
    std::string host;
    int port;
    std::string role;
    bool is_healthy;
    int shard_id;
    
    NodeInfo() : port(0), is_healthy(true), shard_id(-1) {}
    
    std::string address() const {
        return host + ":" + std::to_string(port);
    }
};

class ClusterConfig {
public:
    static ClusterConfig& getInstance();
    
    // 从文件加载配置
    bool loadFromFile(const std::string& config_file);
    
    // 从JSON字符串加载
    bool loadFromJson(const std::string& json_str);
    
    // 获取节点（根据key哈希）
    NodeInfo getNodeByKey(const std::string& key);
    
    // 获取所有节点
    std::vector<NodeInfo> getAllNodes() const;
    
    // 获取节点数量
    size_t getNodeCount() const;
    
    // 标记节点状态
    void markNodeUnhealthy(const std::string& node_id);
    void markNodeHealthy(const std::string& node_id);
    
private:
    ClusterConfig();
    ~ClusterConfig() = default;
    
    // 初始化默认配置
    void initDefaultConfig();
    
    // 解析JSON配置
    bool parseJsonConfig(const std::string& json_str);
    
    std::vector<NodeInfo> nodes_;
    bool config_loaded_ = false;
    std::string config_file_;
};

#endif
