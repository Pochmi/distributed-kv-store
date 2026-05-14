#ifndef CONSISTENT_HASH_H
#define CONSISTENT_HASH_H

#include <map>
#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <iostream>

class ConsistentHash {
public:
    struct Node {
        std::string id;
        std::string host;
        int port;
        std::string role;
        
        Node() : port(0) {}
        Node(const std::string& i, const std::string& h, int p, const std::string& r = "")
            : id(i), host(h), port(p), role(r) {}
    };
    
    ConsistentHash(int virtual_nodes = 300) : virtual_nodes_(virtual_nodes) {}
    
    void addNode(const Node& node) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (int i = 0; i < virtual_nodes_; i++) {
            std::string vkey = node.id + "#" + std::to_string(i);
            uint32_t hash = std::hash<std::string>{}(vkey);
            ring_[hash] = node;
        }
        std::cout << "[ConsistentHash] 添加节点: " << node.id << std::endl;
    }
    
    void removeNode(const std::string& node_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto it = ring_.begin(); it != ring_.end();) {
            if (it->second.id == node_id) {
                it = ring_.erase(it);
            } else {
                ++it;
            }
        }
        std::cout << "[ConsistentHash] 移除节点: " << node_id << std::endl;
    }
    
    Node getNode(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (ring_.empty()) {
            throw std::runtime_error("Hash ring is empty");
        }
        uint32_t hash = std::hash<std::string>{}(key);
        auto it = ring_.lower_bound(hash);
        if (it == ring_.end()) {
            it = ring_.begin();
        }
        return it->second;
    }
    
    void updateNodes(const std::vector<Node>& nodes) {
        std::lock_guard<std::mutex> lock(mutex_);
        ring_.clear();
        for (const auto& node : nodes) {
            for (int i = 0; i < virtual_nodes_; i++) {
                std::string vkey = node.id + "#" + std::to_string(i);
                uint32_t hash = std::hash<std::string>{}(vkey);
                ring_[hash] = node;
            }
        }
        std::cout << "[ConsistentHash] 更新哈希环，节点数: " << nodes.size() 
                  << ", 虚拟节点数: " << ring_.size() << std::endl;
    }
    
    size_t getRingSize() const { return ring_.size(); }
    
private:
    int virtual_nodes_;
    std::map<uint32_t, Node> ring_;
    std::mutex mutex_;
};

#endif
