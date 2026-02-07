#include "common/logger.h"
#include "common/config.h"
#include "core/memory_store.h"
#include "network/simple_server.h"
#include "replication/replica_manager.h"
#include "replication/master.h"
#include "replication/slave.h"
#include "cluster/heartbeat.h"
#include "cluster/failure_detector.h"

#include <iostream>
#include <memory>
#include <string>
#include <csignal>
#include <atomic>
#include <thread>

// Global pointers for signal handling
std::atomic<bool> g_running{true};
std::unique_ptr<simple_server> g_server;
std::unique_ptr<ReplicaManager> g_replica_manager;
std::unique_ptr<HeartbeatManager> g_heartbeat_manager;

// Signal handler
void signal_handler(int signal) {
    Logger::warn("Received signal %d, shutting down...", signal);
    g_running = false;
    
    // Stop components in order
    if (g_heartbeat_manager) {
        g_heartbeat_manager->stop();
    }
    
    if (g_replica_manager) {
        g_replica_manager->stop();
    }
    
    if (g_server) {
        g_server->stop();
    }
}

// Print usage
void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --config <file>     Configuration file (required)" << std::endl;
    std::cout << "  --port <num>        Server port" << std::endl;
    std::cout << "  --host <addr>       Server host" << std::endl;
    std::cout << "  --role <role>       Node role (master/slave)" << std::endl;
    std::cout << "  --master <addr:port> Master address (for slaves)" << std::endl;
    std::cout << "  --log-level <level> Log level (DEBUG/INFO/WARN/ERROR)" << std::endl;
    std::cout << "  --help              Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << program_name << " --config configs/master.json" << std::endl;
    std::cout << "  " << program_name << " --port 6380 --role master" << std::endl;
    std::cout << "  " << program_name << " --port 6381 --role slave --master 127.0.0.1:6380" << std::endl;
}

// Parse command line arguments
Config parse_arguments(int argc, char* argv[]) {
    Config config;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            exit(0);
        }
        else if (arg == "--config") {
            if (i + 1 < argc) {
                config = Config::loadFromFile(argv[++i]);
            } else {
                Logger::error("--config requires a file path");
                exit(1);
            }
        }
        else if (arg == "--port") {
            if (i + 1 < argc) {
                config.setPort(std::stoi(argv[++i]));
            }
        }
        else if (arg == "--host") {
            if (i + 1 < argc) {
                config.setHost(argv[++i]);
            }
        }
        else if (arg == "--role") {
            if (i + 1 < argc) {
                config.setRole(argv[++i]);
            }
        }
        else if (arg == "--master") {
            if (i + 1 < argc) {
                std::string master_addr = argv[++i];
                size_t colon_pos = master_addr.find(':');
                if (colon_pos != std::string::npos) {
                    config.setMasterHost(master_addr.substr(0, colon_pos));
                    config.setMasterPort(std::stoi(master_addr.substr(colon_pos + 1)));
                }
            }
        }
        else if (arg == "--log-level") {
            if (i + 1 < argc) {
                config.setLogLevel(argv[++i]);
            }
        }
        else {
            Logger::warn("Unknown argument: %s", arg.c_str());
        }
    }
    
    return config;
}

// Initialize replication manager
std::unique_ptr<ReplicaManager> init_replication(const Config& config, 
                                                 KVStore* store) {
    if (!config.isReplicationEnabled()) {
        Logger::info("Replication is disabled");
        return nullptr;
    }
    
    Logger::info("Initializing replication manager...");
    
    // Determine role
    ReplicaManager::Role role;
    if (config.getRole() == "master") {
        role = ReplicaManager::MASTER;
        Logger::info("Node role: MASTER");
    } else if (config.getRole() == "slave") {
        role = ReplicaManager::SLAVE;
        Logger::info("Node role: SLAVE");
    } else {
        Logger::error("Unknown role: %s", config.getRole().c_str());
        return nullptr;
    }
    
    // Create replication manager
    auto replica_mgr = std::make_unique<ReplicaManager>(
        store, 
        role, 
        config.getNodeId()
    );
    
    // Configure based on role
    if (role == ReplicaManager::SLAVE) {
        // Slave node: connect to master
        if (!config.getMasterHost().empty() && config.getMasterPort() > 0) {
            replica_mgr->setMaster(config.getMasterHost(), config.getMasterPort());
            Logger::info("Configured master: %s:%d", 
                        config.getMasterHost().c_str(), 
                        config.getMasterPort());
        } else {
            Logger::warn("Slave node but no master specified");
        }
    } else {
        // Master node: add slaves
        const auto& slaves = config.getSlaves();
        for (const auto& slave : slaves) {
            replica_mgr->addSlave(slave.host, slave.port);
            Logger::info("Added slave: %s:%d", slave.host.c_str(), slave.port);
        }
    }
    
    return replica_mgr;
}

// Initialize heartbeat manager
std::unique_ptr<HeartbeatManager> init_heartbeat(const Config& config) {
    if (!config.isClusterEnabled()) {
        return nullptr;
    }
    
    Logger::info("Initializing heartbeat manager...");
    
    auto heartbeat_mgr = std::make_unique<HeartbeatManager>(
        config.getNodeId(),
        config.getHeartbeatIntervalMs(),
        config.getHeartbeatTimeoutMs()
    );
    
    // Add cluster nodes to monitor
    const auto& cluster_nodes = config.getClusterNodes();
    for (const auto& node : cluster_nodes) {
        if (node.id != config.getNodeId()) {  // Don't monitor self
            heartbeat_mgr->addNode(node.id, node.host, node.port);
            Logger::info("Monitoring node: %s (%s:%d)", 
                        node.id.c_str(), node.host.c_str(), node.port);
        }
    }
    
    return heartbeat_mgr;
}

// Request handler with replication support
class ReplicationAwareHandler : public RequestHandler {
public:
    ReplicationAwareHandler(KVStore* store, ReplicaManager* replica_mgr)
        : store_(store), replica_mgr_(replica_mgr) {}
    
    std::string handleRequest(const std::string& request) override {
        // Parse request
        std::string cmd, key, value;
        if (!parseCommand(request, cmd, key, value)) {
            return "ERROR Invalid command format";
        }
        
        // Handle different commands
        if (cmd == "PUT") {
            return handlePut(key, value);
        }
        else if (cmd == "GET") {
            return handleGet(key);
        }
        else if (cmd == "DELETE") {
            return handleDelete(key);
        }
        else if (cmd == "REPLICATION_STATUS") {
            return handleReplicationStatus();
        }
        else if (cmd == "NODE_STATUS") {
            return handleNodeStatus();
        }
        else if (cmd == "PING") {
            return "PONG";
        }
        else {
            return "ERROR Unknown command: " + cmd;
        }
    }
    
private:
    bool parseCommand(const std::string& request, 
                     std::string& cmd, 
                     std::string& key, 
                     std::string& value) {
        size_t space1 = request.find(' ');
        if (space1 == std::string::npos) {
            cmd = request;
            return true;
        }
        
        cmd = request.substr(0, space1);
        
        size_t space2 = request.find(' ', space1 + 1);
        if (space2 == std::string::npos) {
            key = request.substr(space1 + 1);
            return true;
        }
        
        key = request.substr(space1 + 1, space2 - space1 - 1);
        value = request.substr(space2 + 1);
        return true;
    }
    
    std::string handlePut(const std::string& key, const std::string& value) {
        // If we have replication manager, use it
        if (replica_mgr_ && replica_mgr_->getRole() == ReplicaManager::MASTER) {
            if (replica_mgr_->handleWrite(key, value, false)) {
                return "OK";
            } else {
                return "ERROR Replication failed";
            }
        }
        // Otherwise write directly
        else if (store_->put(key, value)) {
            return "OK";
        } else {
            return "ERROR Write failed";
        }
    }
    
    std::string handleGet(const std::string& key) {
        std::string value;
        if (store_->get(key, value)) {
            return "OK " + value;
        } else {
            return "ERROR Key not found";
        }
    }
    
    std::string handleDelete(const std::string& key) {
        // If we have replication manager, use it
        if (replica_mgr_ && replica_mgr_->getRole() == ReplicaManager::MASTER) {
            if (replica_mgr_->handleWrite(key, "", true)) {
                return "OK";
            } else {
                return "ERROR Replication failed";
            }
        }
        // Otherwise delete directly
        else if (store_->deleteKey(key)) {
            return "OK";
        } else {
            return "ERROR Delete failed";
        }
    }
    
    std::string handleReplicationStatus() {
        if (!replica_mgr_) {
            return "ERROR Replication not enabled";
        }
        
        return replica_mgr_->getStatus();
    }
    
    std::string handleNodeStatus() {
        std::string status = "Node: " + (replica_mgr_ ? 
            (replica_mgr_->getRole() == ReplicaManager::MASTER ? "MASTER" : "SLAVE") : 
            "STANDALONE");
        
        if (replica_mgr_) {
            status += "\n" + replica_mgr_->getStatus();
        }
        
        return status;
    }
    
private:
    KVStore* store_;
    ReplicaManager* replica_mgr_;
};

// Main function
int main(int argc, char* argv[]) {
    // Parse command line arguments
    Config config = parse_arguments(argc, argv);
    
    // Validate configuration
    if (config.getPort() == 0) {
        Logger::error("Port not specified. Use --port or config file");
        print_usage(argv[0]);
        return 1;
    }
    
    // Set log level
    Logger::setLevelFromString(config.getLogLevel());
    
    // Print startup banner
    Logger::info("========================================");
    Logger::info("   Distributed KV Store - Server");
    Logger::info("   Version: 3.0.0 (Phase 3: Replication)");
    Logger::info("========================================");
    Logger::info("Node ID: %s", config.getNodeId().c_str());
    Logger::info("Host: %s", config.getHost().c_str());
    Logger::info("Port: %d", config.getPort());
    Logger::info("Role: %s", config.getRole().c_str());
    Logger::info("Replication: %s", 
                config.isReplicationEnabled() ? "ENABLED" : "DISABLED");
    
    // Initialize storage engine
    Logger::info("Initializing storage engine...");
    auto store = std::make_unique<MemoryStore>();
    
    // Initialize replication manager
    g_replica_manager = init_replication(config, store.get());
    if (g_replica_manager) {
        g_replica_manager->start();
    }
    
    // Initialize heartbeat manager (for cluster monitoring)
    g_heartbeat_manager = init_heartbeat(config);
    if (g_heartbeat_manager) {
        g_heartbeat_manager->start();
    }
    
    // Create request handler
    auto handler = std::make_unique<ReplicationAwareHandler>(
        store.get(), 
        g_replica_manager.get()
    );
    
    // Create and start server
    Logger::info("Starting server on %s:%d...", 
                config.getHost().c_str(), config.getPort());
    
    g_server = std::make_unique<simple_server>(
        config.getHost(),
        config.getPort(),
        std::move(handler)
    );
    
    // Set up signal handlers
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    // Start server in a separate thread
    std::thread server_thread([&]() {
        if (!g_server->start()) {
            Logger::error("Failed to start server");
            g_running = false;
        }
    });
    
    // Main loop
    Logger::info("Server started successfully. Press Ctrl+C to stop.");
    
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // Periodically log status
        static int counter = 0;
        if (++counter % 30 == 0) {  // Every 30 seconds
            if (g_replica_manager) {
                Logger::info("Replication status: %s", 
                           g_replica_manager->getStatus().c_str());
            }
            
            if (g_heartbeat_manager) {
                auto node_status = g_heartbeat_manager->getNodeStatus();
                Logger::info("Cluster nodes: %zu alive, %zu dead", 
                           node_status.alive, node_status.dead);
            }
        }
    }
    
    // Wait for server thread
    if (server_thread.joinable()) {
        server_thread.join();
    }
    
    Logger::info("Server shutdown complete");
    return 0;
}
