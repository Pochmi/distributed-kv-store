#ifndef CLIENT_KV_CLIENT_H
#define CLIENT_KV_CLIENT_H

#include <string>

class KVClient {
public:
    KVClient();
    bool put(const std::string& key, const std::string& value);
    std::string get(const std::string& key);
    bool del(const std::string& key);
    
private:
    bool sendToNode(const std::string& host, int port, 
                   const std::string& request, 
                   std::string& response);
    
    std::string serializeRequest(const std::string& cmd, 
                                const std::string& key, 
                                const std::string& value = "");
};
#endif
