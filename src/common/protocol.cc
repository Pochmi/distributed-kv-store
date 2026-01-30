// src/common/protocol.cc
#include "protocol.h"
#include <sstream>
#include <algorithm>
#include "../common/logger.h"

Request ProtocolParser::ParseRequest(const std::string& raw_request) {
    Request req;
    
    if (raw_request.empty()) {
        return req;
    }
    
    // 移除末尾的换行符
    std::string request = raw_request;
    if (!request.empty() && request.back() == '\n') {
        request.pop_back();
    }
    
    std::vector<std::string> parts = Split(request, ' ');
    if (parts.empty()) {
        return req;
    }
    
    req.type = ParseCommand(parts[0]);
    if (req.type == CMD_UNKNOWN) {
        LOG_WARNING("Unknown command: " + parts[0]);
    }
    
    // 复制参数（跳过命令本身）
    if (parts.size() > 1) {
        req.args.assign(parts.begin() + 1, parts.end());
    }
    
    LOG_DEBUG("Parsed request: " + CommandToString(req.type) + 
              " with " + std::to_string(req.args.size()) + " args");
    return req;
}

std::string ProtocolParser::FormatResponse(const Response& response) {
    std::stringstream ss;
    ss << (response.success ? "OK" : "ERROR");
    
    if (!response.message.empty()) {
        ss << " " << response.message;
    }
    
    if (!response.data.empty()) {
        ss << " " << response.data;
    }
    
    ss << "\n";
    return ss.str();
}

CommandType ProtocolParser::ParseCommand(const std::string& cmd_str) {
    std::string cmd = cmd_str;
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
    
    if (cmd == "SET") return CMD_SET;
    if (cmd == "GET") return CMD_GET;
    if (cmd == "DEL" || cmd == "DELETE") return CMD_DEL;
    if (cmd == "EXISTS") return CMD_EXISTS;
    if (cmd == "PING") return CMD_PING;
    if (cmd == "QUIT" || cmd == "EXIT") return CMD_QUIT;
    
    return CMD_UNKNOWN;
}

std::string ProtocolParser::CommandToString(CommandType cmd) {
    switch (cmd) {
        case CMD_SET: return "SET";
        case CMD_GET: return "GET";
        case CMD_DEL: return "DEL";
        case CMD_EXISTS: return "EXISTS";
        case CMD_PING: return "PING";
        case CMD_QUIT: return "QUIT";
        default: return "UNKNOWN";
    }
}

std::vector<std::string> ProtocolParser::Split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, delimiter)) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    
    return tokens;
}
