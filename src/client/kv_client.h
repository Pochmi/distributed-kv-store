#ifndef KV_CLIENT_H
#define KV_CLIENT_H

#include "connection.h"
#include "router.h"
#include "common/protocol.h"
#include <string>
#include <memory>

class KVClient {
public:
    KVClient();
    ~KVClient();
    
    // 基本操作
    bool put(const std::string& key, const std::string& value);
    std::string get(const std::string& key);
    bool del(const std::string& key);
    
    // 批量操作
    bool batchPut(const std::vector<std::pair<std::string, std::string>>& kvs);
    
    // 测试连接
    bool ping();
    
private:
    std::unique_ptr<Router> router_;
    std::unique_ptr<Connection> current_connection_;
    
    // 连接到指定节点
    bool connectToNode(const NodeInfo& node);
    
    // 执行命令
    std::string executeCommand(const std::string& command);
    
    // 重试机制
    std::string executeWithRetry(const std::string& command, const std::string& key, int max_retries = 3);
};

#endif
