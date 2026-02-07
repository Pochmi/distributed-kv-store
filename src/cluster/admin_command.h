#ifndef ADMIN_COMMAND_H
#define ADMIN_COMMAND_H

#include <string>
#include <functional>
#include <map>

class AdminCommandHandler {
public:
    using CommandHandler = std::function<std::string(const std::vector<std::string>& args)>;
    
    AdminCommandHandler();
    
    void registerCommand(const std::string& command, CommandHandler handler);
    std::string handleCommand(const std::string& command_line);
    
    static AdminCommandHandler& getInstance();
    
private:
    std::map<std::string, CommandHandler> commands_;
    
    // 内置命令处理函数
    std::string handleStatus(const std::vector<std::string>& args);
    std::string handleNodes(const std::vector<std::string>& args);
    std::string handlePromote(const std::vector<std::string>& args);
    std::string handleDemote(const std::vector<std::string>& args);
    std::string handleFailover(const std::vector<std::string>& args);
    std::string handleHelp(const std::vector<std::string>& args);
};

#endif // ADMIN_COMMAND_H
