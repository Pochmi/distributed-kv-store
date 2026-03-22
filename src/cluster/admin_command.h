#ifndef CLUSTER_ADMIN_COMMAND_H
#define CLUSTER_ADMIN_COMMAND_H

#include <string>
#include <vector>
#include <map>
#include <functional>

class AdminCommandHandler {
public:
    using CommandFunc = std::function<std::string(const std::vector<std::string>&)>;
    
    AdminCommandHandler();
    ~AdminCommandHandler() = default;
    
    // 注册命令
    void registerCommand(const std::string& name, CommandFunc func);
    
    // 处理命令
    std::string handleCommand(const std::string& cmd_line);
    
    // 获取帮助
    std::string getHelp() const;
    
private:
    std::map<std::string, CommandFunc> commands_;
    
    // 内置命令处理函数
    std::string handlePing(const std::vector<std::string>& args);
    std::string handleStatus(const std::vector<std::string>& args);
    std::string handleNodes(const std::vector<std::string>& args);
    std::string handleFailover(const std::vector<std::string>& args);
    std::string handleHelp(const std::vector<std::string>& args);
};

#endif