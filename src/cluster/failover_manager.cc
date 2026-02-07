#include "failover_manager.h"
#include "common/logger.h"

#include <thread>
#include <chrono>
#include <sstream>
#include <algorithm>

FailoverManager::FailoverManager(const std::string& current_node_id)
    : current_node_id_(current_node_id)
    , monitoring_(false) {
    
    Logger::info("FailoverManager initialized for node: %s", current_node_id.c_str());
}

FailoverManager::~FailoverManager() {
    stopMonitoring();
}

void FailoverManager::addNode(const ClusterNode& node) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (cluster_nodes_.find(node.id) != cluster_nodes_.end()) {
        Logger::warn("Node %s already exists in cluster", node.id.c_str());
        return;
    }
    
    cluster_nodes_[node.id] = node;
    Logger::info("Added node %s to cluster: %s:%d [%s]", 
                node.id.c_str(), node.host.c_str(), node.port, node.role.c_str());
}

void FailoverManager::removeNode(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = cluster_nodes_.find(node_id);
    if (it != cluster_nodes_.end()) {
        Logger::info("Removed node %s from cluster", node_id.c_str());
        cluster_nodes_.erase(it);
    } else {
        Logger::warn("Node %s not found in cluster", node_id.c_str());
    }
}

void FailoverManager::startMonitoring() {
    if (monitoring_) {
        Logger::warn("FailoverManager already monitoring");
        return;
    }
    
    monitoring_ = true;
    monitor_thread_ = std::thread(&FailoverManager::monitorThreadFunc, this);
    Logger::info("FailoverManager started monitoring");
}

void FailoverManager::stopMonitoring() {
    if (!monitoring_) {
        return;
    }
    
    monitoring_ = false;
    
    if (monitor_thread_.joinable()) {
        monitor_thread_.join();
    }
    
    Logger::info("FailoverManager stopped monitoring");
}

bool FailoverManager::promoteSlaveToMaster(const std::string& slave_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 查找要提升的从节点
    auto slave_it = cluster_nodes_.find(slave_id);
    if (slave_it == cluster_nodes_.end()) {
        Logger::error("Slave %s not found in cluster", slave_id.c_str());
        return false;
    }
    
    if (slave_it->second.role != "slave") {
        Logger::error("Node %s is not a slave (role: %s)", 
                     slave_id.c_str(), slave_it->second.role.c_str());
        return false;
    }
    
    // 查找当前的主节点
    std::string old_master_id;
    for (const auto& kv : cluster_nodes_) {
        if (kv.second.role == "master") {
            old_master_id = kv.first;
            break;
        }
    }
    
    if (!old_master_id.empty()) {
        // 将原主节点降级为从节点
        auto& old_master = cluster_nodes_[old_master_id];
        old_master.role = "slave";
        Logger::info("Demoted old master %s to slave", old_master_id.c_str());
    }
    
    // 提升从节点为主节点
    slave_it->second.role = "master";
    slave_it->second.priority = 100;  // 主节点优先级最高
    
    Logger::info("Promoted slave %s to master", slave_id.c_str());
    
    // 这里应该通知所有节点更新配置
    // 在实际实现中，会通过网络通知其他节点
    
    return true;
}

bool FailoverManager::demoteMasterToSlave(const std::string& master_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto master_it = cluster_nodes_.find(master_id);
    if (master_it == cluster_nodes_.end()) {
        Logger::error("Master %s not found in cluster", master_id.c_str());
        return false;
    }
    
    if (master_it->second.role != "master") {
        Logger::error("Node %s is not a master (role: %s)", 
                     master_id.c_str(), master_it->second.role.c_str());
        return false;
    }
    
    // 降级为主节点为从节点
    master_it->second.role = "slave";
    master_it->second.priority = 50;  // 默认优先级
    
    Logger::info("Demoted master %s to slave", master_id.c_str());
    
    // 需要选举新的主节点
    ClusterNode new_master = selectNewMaster();
    if (!new_master.id.empty()) {
        auto& node = cluster_nodes_[new_master.id];
        node.role = "master";
        node.priority = 100;
        Logger::info("Elected new master: %s", new_master.id.c_str());
    }
    
    return true;
}

std::string FailoverManager::getMasterId() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (const auto& kv : cluster_nodes_) {
        if (kv.second.role == "master") {
            return kv.first;
        }
    }
    
    return "";
}

std::vector<std::string> FailoverManager::getSlaveIds() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> slave_ids;
    
    for (const auto& kv : cluster_nodes_) {
        if (kv.second.role == "slave") {
            slave_ids.push_back(kv.first);
        }
    }
    
    return slave_ids;
}

std::string FailoverManager::getClusterStatus() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::stringstream ss;
    ss << "Cluster Status:\n";
    ss << "  Current node: " << current_node_id_ << "\n";
    ss << "  Total nodes: " << cluster_nodes_.size() << "\n";
    
    int master_count = 0;
    int slave_count = 0;
    int alive_count = 0;
    
    for (const auto& kv : cluster_nodes_) {
        const auto& node = kv.second;
        if (node.role == "master") master_count++;
        if (node.role == "slave") slave_count++;
        if (node.is_alive) alive_count++;
    }
    
    ss << "  Masters: " << master_count << "\n";
    ss << "  Slaves: " << slave_count << "\n";
    ss << "  Alive nodes: " << alive_count << "\n";
    ss << "  Dead nodes: " << (cluster_nodes_.size() - alive_count) << "\n";
    
    ss << "\nNode Details:\n";
    for (const auto& kv : cluster_nodes_) {
        const auto& node = kv.second;
        ss << "  - " << node.id << " [" << node.host << ":" << node.port 
           << "] role=" << node.role 
           << " priority=" << node.priority
           << " alive=" << (node.is_alive ? "true" : "false") << "\n";
    }
    
    return ss.str();
}

void FailoverManager::monitorThreadFunc() {
    Logger::info("Cluster monitor thread started");
    
    while (monitoring_) {
        std::this_thread::sleep_for(std::chrono::seconds(3));
        
        try {
            detectMasterFailure();
        } catch (const std::exception& e) {
            Logger::error("Error in monitor thread: %s", e.what());
        }
    }
    
    Logger::info("Cluster monitor thread stopped");
}

void FailoverManager::detectMasterFailure() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 查找主节点
    std::string master_id;
    for (const auto& kv : cluster_nodes_) {
        if (kv.second.role == "master") {
            master_id = kv.first;
            break;
        }
    }
    
    if (master_id.empty()) {
        Logger::warn("No master found in cluster");
        return;
    }
    
    auto& master = cluster_nodes_[master_id];
    
    // 检查主节点是否存活
    if (!master.is_alive) {
        Logger::warn("Master %s is dead, initiating failover...", master_id.c_str());
        
        // 选择新的主节点
        ClusterNode new_master = selectNewMaster();
        if (!new_master.id.empty()) {
            // 执行故障切换
            promoteSlaveToMaster(new_master.id);
            
            // 更新原主节点状态
            master.role = "slave";
            master.priority = 10;  // 降低优先级
            
            Logger::info("Failover completed: %s is new master", new_master.id.c_str());
        } else {
            Logger::error("No suitable slave found for failover");
        }
    }
}

ClusterNode FailoverManager::selectNewMaster() {
    // 选择策略：优先级最高的存活从节点
    std::vector<ClusterNode> candidates;
    
    for (const auto& kv : cluster_nodes_) {
        const auto& node = kv.second;
        if (node.role == "slave" && node.is_alive) {
            candidates.push_back(node);
        }
    }
    
    if (candidates.empty()) {
        return ClusterNode();
    }
    
    // 按优先级排序（降序）
    std::sort(candidates.begin(), candidates.end(), 
              [](const ClusterNode& a, const ClusterNode& b) {
                  return a.priority > b.priority;
              });
    
    return candidates[0];
}

bool FailoverManager::initiateElection() {
    Logger::info("Initiating election for new master...");
    
    // 简化版选举：当前节点发起选举
    // 在实际实现中，会使用类似Raft的选举算法
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 检查当前节点是否有资格成为主节点
    auto it = cluster_nodes_.find(current_node_id_);
    if (it == cluster_nodes_.end() || !it->second.is_alive) {
        Logger::error("Current node %s cannot participate in election", 
                     current_node_id_.c_str());
        return false;
    }
    
    // 模拟选举过程
    Logger::info("Node %s is requesting votes...", current_node_id_.c_str());
    
    // 这里应该发送投票请求给其他节点
    // 简化版：直接成为候选节点
    it->second.role = "candidate";
    
    // 模拟收集选票
    int votes = 1;  // 自己的一票
    for (const auto& kv : cluster_nodes_) {
        if (kv.first != current_node_id_ && kv.second.is_alive) {
            // 假设获得其他节点的投票
            votes++;
        }
    }
    
    // 检查是否获得多数票
    int total_alive = 0;
    for (const auto& kv : cluster_nodes_) {
        if (kv.second.is_alive) total_alive++;
    }
    
    if (votes > total_alive / 2) {
        // 选举成功，成为主节点
        it->second.role = "master";
        it->second.priority = 100;
        
        Logger::info("Election successful! Node %s is now master with %d/%d votes", 
                    current_node_id_.c_str(), votes, total_alive);
        
        // 通知其他节点
        for (auto& kv : cluster_nodes_) {
            if (kv.first != current_node_id_) {
                kv.second.role = "slave";
            }
        }
        
        return true;
    } else {
        // 选举失败
        it->second.role = "slave";
        Logger::info("Election failed: only %d/%d votes", votes, total_alive);
        return false;
    }
}
