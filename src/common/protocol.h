// src/common/protocol.h
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <string>
#include <vector>

// 简单文本协议
// 请求格式：COMMAND [ARG1] [ARG2] ...\n
// 响应格式：STATUS [MESSAGE]\n

enum CommandType {
    CMD_UNKNOWN = 0,
    CMD_SET = 1,
    CMD_GET = 2,
    CMD_DEL = 3,
    CMD_EXISTS = 4,
    CMD_PING = 5,
    CMD_QUIT = 6
};

struct Request {
    CommandType type;
    std::vector<std::string> args;
    
    Request() : type(CMD_UNKNOWN) {}
};

struct Response {
    bool success;
    std::string message;
    std::string data;
    
    Response(bool s = true, const std::string& m = "", const std::string& d = "") 
        : success(s), message(m), data(d) {}
};

class ProtocolParser {
public:
    static Request ParseRequest(const std::string& raw_request);
    static std::string FormatResponse(const Response& response);
    static CommandType ParseCommand(const std::string& cmd_str);
    static std::string CommandToString(CommandType cmd);
    
private:
    static std::vector<std::string> Split(const std::string& str, char delimiter);
};

#endif // PROTOCOL_H
