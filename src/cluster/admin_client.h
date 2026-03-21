#ifndef ADMIN_CLIENT_H
#define ADMIN_CLIENT_H

#include <string>
#include <vector>
#include <map>

struct NodeStatus {
    std::string id;
    std::string host;
    int port;
    std::string role;
    bool is_alive;
    uint64_t last_heartbeat;
};

struct ClusterStats {
    int total_nodes;
    int alive_nodes;
    int dead_nodes;
    std::string master_id;
    std::vector<std::string> slave_ids;
};

class AdminCommandClient {
public:
    AdminCommandClient(const std::string& master_addr = "");
    ~AdminCommandClient();
    
    bool connect(const std::string& master_addr);
    void disconnect();
    
    std::vector<NodeStatus> listNodes();
    ClusterStats getStats();
    bool initiateFailover();
    bool promoteSlave(const std::string& slave_id);
    bool demoteMaster(const std::string& master_id);
    
    std::string getLastError() const { return last_error_; }
    
private:
    bool sendCommand(const std::string& cmd, std::string& response);
    
    std::string master_addr_;
    int socket_fd_;
    bool connected_;
    std::string last_error_;
};

#endif // ADMIN_CLIENT_H
