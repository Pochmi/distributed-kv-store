#include "common/logger.h"
#include "client/cluster_config.h"
#include "core/kv_store.h"
#include "network/simple_server.h"
#include "replication/replica_manager.h"
#include "cluster/heartbeat.h"
#include "cluster/failover_manager.h"
#include <iostream>
#include <memory>
#include <csignal>
#include <thread>
#include <getopt.h>

// 全局变量
std::unique_ptr<SimpleServer> g_server;
std::unique_ptr<KVStore> g_store;
std::unique_ptr<ReplicaManager> g_replica_mgr;
std::unique_ptr<HeartbeatManager> g_heartbeat;
std::unique_ptr<FailoverManager> g_failover;

// 配置
struct ServerConfig {
    int port = 6380;
    std::string host = "0.0.0.0";
    std::string role = "master";
    std::string master_addr;
    bool replication_enabled = false;
    bool cluster_enabled = false;
    std::string config_file;
    std::string log_level = "info";
};

// 信号处理函数
void signal_handler(int signal) {
    Logger::instance().warning("Received signal " + std::to_string(signal) + ", shutting down...");
    
    if (g_replica_mgr) {
        g_replica_mgr->stop();
    }
    
    if (g_server) {
        g_server->Stop();
    }
    
    exit(0);
}

// 自定义请求处理器
class ReplicationAwareHandler {
public:
    ReplicationAwareHandler(KVStore* store, ReplicaManager* replica_mgr)
        : store_(store), replica_mgr_(replica_mgr) {}
    
    std::string operator()(const std::string& request) {
    std::cout << "[DEBUG] Received:  << request << " << std::endl;
        // 解析请求
        if (request.substr(0, 3) == "SET") {
            size_t space1 = request.find(' ', 4);
            if (space1 == std::string::npos) {
                return "ERROR: Invalid SET format";
            }
            std::string key = request.substr(4, space1 - 4);
            std::string value = request.substr(space1 + 1);
            while (!value.empty() && (value.back() == '\n' || value.back() == '\r')) {
                value.pop_back();
            }
            return handleSet(key, value);
        }
        else if (request.substr(0, 3) == "GET") {
            std::string key = request.substr(4);
            while (!key.empty() && (key.back() == '\n' || key.back() == '\r')) {
                key.pop_back();
            }
            return handleGet(key);
        }
        else if (request.substr(0, 3) == "DEL") {
            std::string key = request.substr(4);
            while (!key.empty() && (key.back() == '\n' || key.back() == '\r')) {
                key.pop_back();
            }
            return handleDelete(key);
        }
        else if (request == "STATUS" || request == "STATUS\n") {
            return handleStatus();
        }
        // ========== 管理命令 ==========
        else if (request.substr(0, 12) == "ADMIN_STATUS") {
            return handleAdminStatus();
        }
        else if (request.substr(0, 14) == "ADMIN_FAILOVER") {
            // 解析参数: ADMIN_FAILOVER <new_master>
            std::string rest = request.substr(15);
            while (!rest.empty() && (rest.back() == '\n' || rest.back() == '\r')) {
                rest.pop_back();
            }
            return handleAdminFailover(rest);
        }
        else if (request.substr(0, 10) == "ADMIN_HELP") {
            return handleAdminHelp();
        }
        // ===========================
        else {
            return "ERROR: Unknown command\n";
        }
    }
    
private:
    std::string handleSet(const std::string& key, const std::string& value) {
        if (replica_mgr_ && replica_mgr_->role() == ReplicaManager::MASTER) {
            if (replica_mgr_->handleWrite(key, value, false)) {
                return "OK\n";
            } else {
                return "ERROR: Failed to write\n";
            }
        }
        else if (store_->Put(key, value).ok()) {
            return "OK\n";
        }
        else {
            return "ERROR: Failed to write\n";
        }
    }
    
    std::string handleGet(const std::string& key) {
        std::string value;
        Status status = store_->Get(key, value);
        if (status.ok()) {
            return "OK " + value + "\n";
        } else if (status.is_key_not_found()) {
            return "NOT_FOUND\n";
        } else {
            return "ERROR: " + status.message + "\n";
        }
    }
    
    std::string handleDelete(const std::string& key) {
        if (replica_mgr_ && replica_mgr_->role() == ReplicaManager::MASTER) {
            if (replica_mgr_->handleWrite(key, "", true)) {
                return "OK\n";
            } else {
                return "ERROR: Failed to delete\n";
            }
        }
        else if (store_->Delete(key).ok()) {
            return "OK\n";
        }
        else {
            return "ERROR: Failed to delete\n";
        }
    }
    
    std::string handleStatus() {
        std::string status = "Server Status:\n";
        status += "  Store size: " + std::to_string(store_->Size()) + "\n";
        
        if (replica_mgr_) {
            status += "  Replication: enabled\n";
            status += replica_mgr_->getStatus();
        } else {
            status += "  Replication: disabled\n";
        }
        
        return status;
    }
    
    // ========== 管理命令处理函数 ==========
    std::string handleAdminStatus() {
        std::string result = "=== Cluster Status ===\n";
        if (g_failover) {
            result += "Current Master: " + g_failover->getMaster() + "\n";
        }
        if (g_heartbeat) {
            result += g_heartbeat->getStatus();
        }
        result += "=======================\n";
        return result;
    }
    
    std::string handleAdminFailover(const std::string& new_master) {
        if (!g_failover) {
            return "ERROR: Failover manager not initialized\n";
        }
        
        Logger::instance().info("Manual failover requested to " + new_master);
        
        if (g_failover->manualFailover(new_master)) {
            return "OK: Failover completed to " + new_master + "\n";
        } else {
            return "ERROR: Failover failed. Node " + new_master + " is not alive\n";
        }
    }
    
    std::string handleAdminHelp() {
        std::string help = "=== Admin Commands ===\n";
        help += "  ADMIN_STATUS           - Show cluster status\n";
        help += "  ADMIN_FAILOVER <node>  - Manual failover to node\n";
        help += "  ADMIN_HELP             - Show this help\n";
        help += "=======================\n";
        return help;
    }
    // =====================================
    
    KVStore* store_;
    ReplicaManager* replica_mgr_;
};

// 解析命令行参数
ServerConfig parse_arguments(int argc, char* argv[]) {
    ServerConfig config;
    
    static struct option long_options[] = {
        {"port", required_argument, 0, 'p'},
        {"host", required_argument, 0, 'h'},
        {"role", required_argument, 0, 'r'},
        {"master", required_argument, 0, 'm'},
        {"config", required_argument, 0, 'c'},
        {"log-level", required_argument, 0, 'l'},
        {"replication", no_argument, 0, 'R'},
        {"cluster", no_argument, 0, 'C'},
        {0, 0, 0, 0}
    };
    
    int opt;
    int option_index = 0;
    
    while ((opt = getopt_long(argc, argv, "p:h:r:m:c:l:RC", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'p':
                config.port = std::stoi(optarg);
                break;
            case 'h':
                config.host = optarg;
                break;
            case 'r':
                config.role = optarg;
                break;
            case 'm':
                config.master_addr = optarg;
                config.replication_enabled = true;
                break;
            case 'c':
                config.config_file = optarg;
                break;
            case 'l':
                config.log_level = optarg;
                break;
            case 'R':
                config.replication_enabled = true;
                break;
            case 'C':
                config.cluster_enabled = true;
                break;
            default:
                std::cerr << "Usage: " << argv[0] 
                          << " [--port PORT] [--host HOST] [--role master|slave]"
                          << " [--master HOST:PORT] [--config FILE] [--log-level LEVEL]"
                          << " [--replication] [--cluster]" << std::endl;
                exit(1);
        }
    }
    
    return config;
}

int main(int argc, char* argv[]) {
    // 解析参数
    ServerConfig config = parse_arguments(argc, argv);
    
    std::string node_id = "node_" + std::to_string(config.port);

    if (config.port == 0) {
        std::cerr << "Error: Port not specified" << std::endl;
        return 1;
    }
    
    // 设置日志级别
    if (config.log_level == "debug") {
        Logger::instance().setLevel(LOG_DEBUG);
    } else if (config.log_level == "info") {
        Logger::instance().setLevel(LOG_INFO);
    } else if (config.log_level == "warning") {
        Logger::instance().setLevel(LOG_WARNING);
    } else if (config.log_level == "error") {
        Logger::instance().setLevel(LOG_ERROR);
    }
    
    // 打印启动信息
    Logger::instance().info("========================================");
    Logger::instance().info("   Distributed KV Store - Server");
    Logger::instance().info("   Version: 4.0.0 (Phase 4: Failover)");
    Logger::instance().info("========================================");
    Logger::instance().info("Host: " + config.host);
    Logger::instance().info("Port: " + std::to_string(config.port));
    Logger::instance().info("Role: " + config.role);
    Logger::instance().info("Replication: " + std::string(config.replication_enabled ? "enabled" : "disabled"));
    
    // 初始化存储引擎
    Logger::instance().info("Initializing storage engine...");
    g_store = KVStore::CreateMemoryStore();
    
    // 初始化复制管理器
    if (config.replication_enabled) {
        Logger::instance().info("Initializing replication manager...");
        
        ReplicaManager::Role role = (config.role == "master") ? 
                                    ReplicaManager::MASTER : ReplicaManager::SLAVE;
        
        g_replica_mgr = std::make_unique<ReplicaManager>(
            g_store.get(), role, node_id);
        
        if (role == ReplicaManager::SLAVE && !config.master_addr.empty()) {
            size_t colon_pos = config.master_addr.find(':');
            if (colon_pos != std::string::npos) {
                std::string master_host = config.master_addr.substr(0, colon_pos);
                int master_port = std::stoi(config.master_addr.substr(colon_pos + 1));
                g_replica_mgr->setMaster(master_host, master_port);
                Logger::instance().info("Configured master: " + master_host + ":" + 
                                       std::to_string(master_port));
            }
        }
    } else {
        Logger::instance().info("Replication is disabled");
    }
    
    // 初始化故障检测模块
    if (config.cluster_enabled) {
        Logger::instance().info("Initializing cluster heartbeat...");
        g_heartbeat = std::make_unique<HeartbeatManager>(node_id, config.port);
        g_failover = std::make_unique<FailoverManager>(g_heartbeat.get());
        
        // 添加集群节点
        g_heartbeat->addNode("node_6381", "127.0.0.1", 6381);
        g_heartbeat->addNode("node_6382", "127.0.0.1", 6382);
        g_heartbeat->addNode("node_6383", "127.0.0.1", 6383);
        
        g_heartbeat->start();
        
        if (config.role == "master") {
            g_failover->setMaster(node_id);
        }
        Logger::instance().info("Cluster heartbeat started");
    }
    
    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 创建请求处理器
    ReplicationAwareHandler handler(g_store.get(), g_replica_mgr.get());
    
    // 创建并启动服务器
    Logger::instance().info("Starting server on " + config.host + ":" + 
                           std::to_string(config.port) + "...");
    
    g_server = std::make_unique<SimpleServer>(
        config.port, 
        std::shared_ptr<KVStore>(g_store.get(), [](KVStore*){}),
        handler);
    
    if (!g_server->Start()) {
        Logger::instance().error("Failed to start server");
        return 1;
    }
    
    Logger::instance().info("Server started successfully. Press Ctrl+C to stop.");
    
    // 启动复制
    if (g_replica_mgr) {
        g_replica_mgr->start();
    }
    
    // 主循环
    Logger::instance().info("Entering main loop...");
    
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // 定期检查故障
        if (g_failover) {
            g_failover->checkAndFailover();
        }
        
        static int counter = 0;
        if (++counter % 10 == 0) {
            Logger::instance().debug("Server running...");
        }
    }
    
    return 0;
}