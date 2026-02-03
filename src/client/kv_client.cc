#include "kv_client.h"
#include "connection.h"
#include "router.h"
#include <iostream>

KVClient::KVClient() {
    std::cout << "[KVClient] Initialized" << std::endl;
}

bool KVClient::put(const std::string& key, const std::string& value) {
    std::cout << "[KVClient] PUT " << key << " = " << value << std::endl;
    
    Router router;
    NodeInfo node;
    try {
        node = router.route(key);
    } catch (const std::exception& e) {
        std::cerr << "[KVClient] Routing failed: " << e.what() << std::endl;
        return false;
    }
    
    std::string request = serializeRequest("SET", key, value);
    std::string response;
    
    if (!sendToNode(node.host, node.port, request, response)) {
        return false;
    }
    
    std::cout << "[KVClient] Server response: " << response;
    return response.find("OK") != std::string::npos;
}

std::string KVClient::get(const std::string& key) {
    std::cout << "[KVClient] GET " << key << std::endl;
    
    Router router;
    NodeInfo node;
    try {
        node = router.route(key);
    } catch (const std::exception& e) {
        std::cerr << "[KVClient] Routing failed: " << e.what() << std::endl;
        return "";
    }
    
    std::string request = serializeRequest("GET", key);
    std::string response;
    
    if (!sendToNode(node.host, node.port, request, response)) {
        return "";
    }
    
    std::cout << "[KVClient] Server response: " << response;
    return response;
}

bool KVClient::del(const std::string& key) {
    std::cout << "[KVClient] DELETE " << key << std::endl;
    
    Router router;
    NodeInfo node;
    try {
        node = router.route(key);
    } catch (const std::exception& e) {
        std::cerr << "[KVClient] Routing failed: " << e.what() << std::endl;
        return false;
    }
    
    std::string request = serializeRequest("DEL", key);
    std::string response;
    
    if (!sendToNode(node.host, node.port, request, response)) {
        return false;
    }
    
    std::cout << "[KVClient] Server response: " << response;
    return response.find("OK") != std::string::npos;
}

bool KVClient::sendToNode(const std::string& host, int port, 
                         const std::string& request, 
                         std::string& response) {
    Connection conn;
    
    if (!conn.connect(host, port)) {
        std::cerr << "[KVClient] Failed to connect to " << host << ":" << port << std::endl;
        return false;
    }
    
    if (!conn.send(request)) {
        std::cerr << "[KVClient] Failed to send request" << std::endl;
        return false;
    }
    
    if (!conn.receive(response)) {
        std::cerr << "[KVClient] Failed to receive response" << std::endl;
        return false;
    }
    
    return true;
}

std::string KVClient::serializeRequest(const std::string& cmd, 
                                      const std::string& key, 
                                      const std::string& value) {
    if (cmd == "SET") {
        return "SET " + key + " " + value + "\n";
    } else if (cmd == "GET") {
        return "GET " + key + "\n";
    } else if (cmd == "DEL") {
        return "DEL " + key + "\n";
    }
    
    return "";
}
