#ifndef CLIENT_CONNECTION_H
#define CLIENT_CONNECTION_H

#include <string>

class Connection {
public:
    Connection();
    ~Connection();
    
    bool connect(const std::string& host, int port);
    bool send(const std::string& data);
    bool receive(std::string& data);
    void close();
    
    bool isConnected() const { return sockfd_ >= 0; }
    
private:
    int sockfd_{-1};
};
#endif
