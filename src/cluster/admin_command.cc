#include "admin_command.h"
#include "../common/logger.h"
#include <sstream>
#include <algorithm>

AdminCommandHandler::AdminCommandHandler() {
    // 注册内置命令
    registerCommand("ping", [this](const auto& args) { return handlePing(args); });
    registerCommand("status", [this](const auto& args) { return handleStatus(args); });
    registerCommand("nodes", [this](const auto& args) { return handleNodes(args); });
    registerCommand("failover", [this](const auto& args) { return handleFailover(args); });
    registerCommand("help", [this](const auto& args) { return handleHelp(args); });
}

void AdminCommandHandler::registerCommand(const std::string& name, CommandFunc func) {
    std::string lower_name = name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
    commands_[lower_name] = func;
    LOG_DEBUG("Registered admin command: " + name);
}

std::string AdminCommandHandler::handleCommand(const std::string& cmd_line) {
    // 解析命令
    std::stringstream ss(cmd_line);
    std::string cmd;
    ss >> cmd;
    
    if (cmd.empty()) return "ERROR: Empty command";
    
    // 转换为小写
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);
    
    // 解析参数
    std::vector<std::string> args;
    std::string arg;
    while (ss >> arg) {
        args.push_back(arg);
    }
    
    auto it = commands_.find(cmd);
    if (it != commands_.end()) {
        return it->second(args);
    }
    
    return "ERROR: Unknown command: " + cmd + "\n" + getHelp();
}

std::string AdminCommandHandler::getHelp() const {
    std::string help = "Available commands:\n";
    for (const auto& pair : commands_) {
        help += "  " + pair.first + "\n";
    }
    return help;
}

std::string AdminCommandHandler::handlePing(const std::vector<std::string>&) {
    return "PONG";
}

std::string AdminCommandHandler::handleStatus(const std::vector<std::string>&) {
    return "STATUS: OK\nServer is running";
}

std::string AdminCommandHandler::handleNodes(const std::vector<std::string>&) {
    return "NODES: master (localhost:6381), slave1 (localhost:6382), slave2 (localhost:6383)";
}

std::string AdminCommandHandler::handleFailover(const std::vector<std::string>& args) {
    if (args.empty()) {
        return "ERROR: failover requires target node";
    }
    return "FAILOVER: switching to " + args[0];
}

std::string AdminCommandHandler::handleHelp(const std::vector<std::string>&) {
    return getHelp();
}