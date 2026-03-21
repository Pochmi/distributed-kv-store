#include "replica_manager.h"
#include "../common/logger.h"
#include "../core/kv_store.h"
#include <sstream>
#include <chrono>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <fcntl.h>
#include <errno.h>

ReplicaManager::ReplicaManager(KVStore* store, Role role, const std::string& node_id)
    : role_(role)
    , node_id_(node_id)
    , store_(store)
    , replication_log_(std::make_unique<ReplicationLog>())
    , master_port_(0)
    , running_(false)
    , slave_socket_(-1) {
    Logger::instance().info("ReplicaManager initialized: " + 
                           std::string(role == MASTER ? "MASTER" : "SLAVE"));
}

ReplicaManager::~ReplicaManager() {
    stop();
}

void ReplicaManager::addSlave(const std::string&, int) {
    // 暂不实现
}

void ReplicaManager::setMaster(const std::string& host, int port) {
    if (role_ != SLAVE) return;
    master_host_ = host;
    master_port_ = port;
    Logger::instance().info("Set master to " + host + ":" + std::to_string(port));
}

void ReplicaManager::start() {
    if (running_) return;
    running_ = true;
    if (role_ == MASTER) {
        replication_thread_ = std::thread(&ReplicaManager::masterReplicationThread, this);
        Logger::instance().info("Master replication thread started");
    } else {
        replication_thread_ = std::thread(&ReplicaManager::slaveSyncThread, this);
        Logger::instance().info("Slave sync thread started");
    }
}

void ReplicaManager::stop() {
    if (!running_) return;
    running_ = false;
    if (slave_socket_ != -1) close(slave_socket_);
    if (replication_thread_.joinable()) replication_thread_.join();
    Logger::instance().info("Replication thread stopped");
}

bool ReplicaManager::handleWrite(const std::string& key, const std::string& value, bool is_delete) {
    if (role_ != MASTER) return false;
    
    ::Status status;
    if (is_delete) {
        status = store_->Delete(key);
    } else {
        status = store_->Put(key, value);
    }
    if (!status.ok()) return false;
    
    LogType type = is_delete ? LogType::DELETE : LogType::PUT;
    replication_log_->append(type, key, value);
    return true;
}

std::string ReplicaManager::getStatus() const {
    std::stringstream ss;
    ss << "Role: " << (role_ == MASTER ? "MASTER" : "SLAVE");
    if (role_ == SLAVE) {
        ss << ", Master: " << master_host_ << ":" << master_port_;
        ss << ", Connected: " << (slave_socket_ != -1 ? "yes" : "no");
    }
    ss << ", Last log ID: " << replication_log_->getLastLogId();
    return ss.str();
}

void ReplicaManager::masterReplicationThread() {
    // 添加一条测试日志
    replication_log_->append(LogType::PUT, "test_key", "test_value");
    Logger::instance().info("Added test log");
    Logger::instance().info("Master replication thread starting...");
    
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        Logger::instance().error("Failed to create listen socket");
        return;
    }
    
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(6380);
    addr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(listen_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        Logger::instance().error("Bind failed");
        close(listen_fd);
        return;
    }
    
    if (listen(listen_fd, 5) < 0) {
        Logger::instance().error("Listen failed");
        close(listen_fd);
        return;
    }
    
    Logger::instance().info("Master replication listening on port 6380");
    
    while (running_) {
        int slave_fd = accept(listen_fd, nullptr, nullptr);
        if (slave_fd < 0) continue;
        
        Logger::instance().info("Slave connected!");
        
        // 等待接收数据
        char buf[4096];
        int n = 0;
        int retry = 0;
        while (retry < 10 && n <= 0) {
            n = recv(slave_fd, buf, sizeof(buf)-1, 0);
            if (n <= 0) {
                usleep(100000); // 等待100ms
                retry++;
            }
        }
        
        if (n > 0) {
            buf[n] = '\0';
            Logger::instance().info("Received: " + std::string(buf));
            
            if (std::string(buf).substr(0, 8) == "SYNC_REQ") {
                auto logs = replication_log_->getEntriesFrom(1, 100);
                Logger::instance().info("Found " + std::to_string(logs.size()) + " logs to send");
                
                for (auto& log : logs) {
                    std::string resp = "LOG " + std::to_string(log.log_id) + " PUT " + log.key + " " + log.value + "\n";
                    send(slave_fd, resp.c_str(), resp.length(), 0);
                    Logger::instance().info("Sent: " + resp);
                }
            }
        } else {
            Logger::instance().error("No data received from slave");
        }
        
        close(slave_fd);
        Logger::instance().info("Slave disconnected");
    }
    
    close(listen_fd);
    Logger::instance().info("Master replication thread stopped");
}

void ReplicaManager::slaveSyncThread() {
    Logger::instance().info("Slave sync thread starting...");
    Logger::instance().info("Target master: " + master_host_ + ":" + std::to_string(master_port_));
    
    int retry_count = 0;
    const int max_retries = 10;
    
    while (running_ && retry_count < max_retries) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            retry_count++;
            sleep(3);
            continue;
        }
        
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(master_port_);
        inet_pton(AF_INET, master_host_.c_str(), &addr.sin_addr);
        
        if (::connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
            Logger::instance().error("Failed to connect to master: " + std::string(strerror(errno)));
            close(sock);
            retry_count++;
            sleep(3);
            continue;
        }
        
        Logger::instance().info("✓ Connected to master!");
        slave_socket_ = sock;
        retry_count = 0;
        
        std::string req = "SYNC_REQ 1\n";
        send(sock, req.c_str(), req.length(), 0);
        
        char buffer[8192];
        int n = recv(sock, buffer, sizeof(buffer)-1, 0);
        if (n > 0) {
            buffer[n] = '\0';
            std::string response(buffer);
            Logger::instance().info("Received " + std::to_string(n) + " bytes");
            
            // 按行分割处理多条日志
            std::stringstream ss(response);
            std::string line;
            while (std::getline(ss, line)) {
                if (line.empty()) continue;
                
                Logger::instance().info("Processing line: " + line);
                
                if (line.substr(0, 3) == "LOG") {
                    size_t p1 = line.find(' ', 4);
                    size_t p2 = line.find(' ', p1 + 1);
                    size_t p3 = line.find(' ', p2 + 1);
                    
                    if (p1 != std::string::npos && p2 != std::string::npos) {
                        try {
                            std::string type = line.substr(p1 + 1, p2 - p1 - 1);
                            std::string key = line.substr(p2 + 1, p3 - p2 - 1);
                            std::string value = line.substr(p3 + 1);
                            
                            if (type == "PUT") {
                                store_->Put(key, value);
                                Logger::instance().info("Replicated: " + key + " = " + value);
                            } else if (type == "DEL") {
                                store_->Delete(key);
                                Logger::instance().info("Replicated DEL: " + key);
                            }
                        } catch (const std::exception& e) {
                            Logger::instance().error("Failed to parse log: " + std::string(e.what()));
                        }
                    }
                }
            }
        }
        
        close(sock);
        slave_socket_ = -1;
        sleep(5);
    }
    
    Logger::instance().info("Slave sync thread stopped");
}

bool ReplicaManager::connectToMaster() { return false; }
bool ReplicaManager::sendLogToSlave(const ReplicaInfo&, const std::vector<ReplicationLogEntry>&) { return false; }
bool ReplicaManager::fetchLogFromMaster(uint64_t) { return false; }
