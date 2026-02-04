#ifndef CONNECTION_H
#define CONNECTION_H

#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>

class Connection {
public:
    Connection(const std::string& host, int port);
    ~Connection();
    
    // 连接服务器
    bool connect();
    
    // 断开连接
    void disconnect();
    
    // 发送数据
    bool send(const std::string& data);
    
    // 接收数据
    std::string receive();
    
    // 是否已连接
    bool isConnected() const;
    
private:
    std::string host_;
    int port_;
    int sockfd_;
    bool connected_;
    
    // 创建socket
    bool createSocket();
};

#endif
