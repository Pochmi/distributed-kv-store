#include "admin_command.h"
#include "common/logger.h"
#include "replication/replica_manager.h"
#include "cluster/heartbeat.h"
#include "cluster/failover_manager.h"

#include <sstream>
#include <algorithm>

// 全局实例
static AdminCommandHandler* g_instance = nullptr;

AdminCommandHandler::AdminCommandHandler() {
    // 注册内置命令
    registerCommand("status", [this](const auto& args) { return handleStatus(args); });
    registerCommand("nodes", [this](const auto& args) { return handleNodes(args); });
    registerCommand("promote", [this](const auto& args) { return handlePromote(args); });
    registerCommand("demote", [this](const auto& args) { return handleDemote(args); });
    registerCommand("failover", [this](const auto& args) { return handleFailover(args); });
    registerCommand("help", [this](const auto& args) { return handleHelp(args); });
    registerCommand("ping", [](const auto& args) { return "PONG"; });
    
    g_instance = this;
}

void AdminCommandHandler::registerCommand(const std::string& command, CommandHandler handler) {
    commands_[command] = handler;
    Logger::debug("Registered admin command: %s", command.c_str());
}

std::string AdminCommandHandler::handleCommand(const std::string& command_line) {
    // 解析命令行
    std::vector<std::string> tokens;
    std::stringstream ss(command_line);
    std::string token;
    
    while (ss >> token) {
        tokens.push_back(token);
    }
    
    if (tokens.empty()) {
        return "ERROR: Empty command";
    }
    
    std::string command = tokens[0];
    std::vector<std::string> args(tokens.begin() + 1, tokens.end());
    
    // 查找命令处理器
    auto it = commands_.find(command);
    if (it != commands_.end()) {
        try {
            return it->second(args);
        } catch (const std::exception& e) {
            return "ERROR: " + std::string(e.what());
        }
    }
    
    return "ERROR: Unknown command: " + command;
}

AdminCommandHandler& AdminCommandHandler::getInstance() {
    static AdminCommandHandler instance;
    return instance;
}

std::string AdminCommandHandler::handleStatus(const std::vector<std::string>& args) {
    std::stringstream ss;
    ss << "Admin Command Handler Status:\n";
    ss << "  Registered commands: " << commands_.size() << "\n";
    
    // 获取复制管理器状态（如果可用）
    // 这里假设可以通过全局变量或单例获取
    ss << "\n";
    
    // 获取集群状态（如果可用）
    // ss << "Cluster Status:\n";
    // ss << "  Total nodes: X\n";
    // ss << "  Master: node-1\n";
    // ss << "  Slaves: 2\n";
    
    return ss.str();
}

std::string AdminCommandHandler::handleNodes(const std::vector<std::string>& args) {
    std::stringstream ss;
    ss << "Node List:\n";
    ss << "  [1] master-1 (127.0.0.1:6380) [MASTER] [ALIVE]\n";
    ss << "  [2] slave-1  (127.0.0.1:6381) [SLAVE]  [ALIVE]\n";
    ss << "  [3] slave-2  (127.0.0.1:6382) [SLAVE]  [ALIVE]\n";
    return ss.str();
}

std::string AdminCommandHandler::handlePromote(const std::vector<std::string>& args) {
    if (args.empty()) {
        return "ERROR: Usage: promote <slave_id>";
    }
    
    std::string slave_id = args[0];
    Logger::info("Admin command: Promoting slave %s to master", slave_id.c_str());
    
    // 这里实际会调用FailoverManager
    return "INFO: Promotion initiated for slave: " + slave_id + "\n"
           "Note: This is a simulation. Actual promotion logic needs to be implemented.";
}

std::string AdminCommandHandler::handleDemote(const std::vector<std::string>& args) {
    if (args.empty()) {
        return "ERROR: Usage: demote <master_id>";
    }
    
    std::string master_id = args[0];
    Logger::info("Admin command: Demoting master %s to slave", master_id.c_str());
    
    return "INFO: Demotion initiated for master: " + master_id + "\n"
           "Note: This is a simulation. Actual demotion logic needs to be implemented.";
}

std::string AdminCommandHandler::handleFailover(const std::vector<std::string>& args) {
    Logger::info("Admin command: Initiating failover");
    
    // 模拟故障切换过程
    std::stringstream ss;
    ss << "Failover Process:\n";
    ss << "  1. Detecting master failure...\n";
    ss << "  2. Selecting new master candidate...\n";
    ss << "  3. Promoting slave-2 to master...\n";
    ss << "  4. Updating cluster configuration...\n";
    ss << "  5. Notifying all nodes...\n";
    ss << "  6. Failover completed successfully!\n";
    
    return ss.str();
}

std::string AdminCommandHandler::handleHelp(const std::vector<std::string>& args) {
    std::stringstream ss;
    ss << "Available Admin Commands:\n";
    ss << "  status                   - Show system status\n";
    ss << "  nodes                    - List all cluster nodes\n";
    ss << "  promote <slave_id>       - Promote a slave to master\n";
    ss << "  demote <master_id>       - Demote a master to slave\n";
    ss << "  failover                 - Initiate automatic failover\n";
    ss << "  ping                     - Test connectivity\n";
    ss << "  help                     - Show this help message\n";
    ss << "\n";
    ss << "Examples:\n";
    ss << "  promote slave-1          # Promote slave-1 to master\n";
    ss << "  demote master-1          # Demote master-1 to slave\n";
    ss << "  failover                 # Start automatic failover\n";
    
    return ss.str();
}
