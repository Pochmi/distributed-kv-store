#ifndef CLUSTER_ADMIN_CLIENT_H
#define CLUSTER_ADMIN_CLIENT_H

#include <string>
#include <vector>

class AdminClient {
public:
    AdminClient(const std::string& host, int port);
    ~AdminClient();
    
    // 发送管理命令
    std::string sendCommand(const std::string& cmd);
    std::string sendCommand(const std::string& cmd, const std::vector<std::string>& args);
    
    // 常用命令
    std::string ping();
    std::string getStatus();
    std::string getNodes();
    std::string failover(const std::string& target_node);
    std::string help();
    
private:
    std::string host_;
    int port_;
    int connect();
    void disconnect(int sock);
};

#endif