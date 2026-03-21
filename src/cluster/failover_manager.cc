#include "failover_manager.h"
#include "../common/logger.h"
#include <algorithm>

FailoverManager::FailoverManager(const std::string& current_node_id)
    : current_node_id_(current_node_id), monitoring_(false) {
    Logger::instance().info("FailoverManager initialized for node: " + current_node_id);
}

FailoverManager::~FailoverManager() {
    stopMonitoring();
}

void FailoverManager::addNode(const ClusterNode& node) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 检查节点是否已存在
    auto it = cluster_nodes_.find(node.id);
    if (it != cluster_nodes_.end()) {
        Logger::instance().warning("Node " + node.id + " already exists in cluster");
        return;
    }
    
    cluster_nodes_[node.id] = node;
    Logger::instance().info("Added node " + node.id + " to cluster: " + 
                           node.host + ":" + std::to_string(node.port) + 
                           " [" + node.role + "]");
}

void FailoverManager::removeNode(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = cluster_nodes_.find(node_id);
    if (it != cluster_nodes_.end()) {
        cluster_nodes_.erase(it);
        Logger::instance().info("Removed node " + node_id + " from cluster");
    } else {
        Logger::instance().warning("Node " + node_id + " not found in cluster");
    }
}

void FailoverManager::startMonitoring() {
    if (monitoring_) {
        Logger::instance().warning("FailoverManager already monitoring");
        return;
    }
    
    monitoring_ = true;
    monitor_thread_ = std::thread(&FailoverManager::monitorThreadFunc, this);
    Logger::instance().info("FailoverManager started monitoring");
}

void FailoverManager::stopMonitoring() {
    if (!monitoring_) {
        return;
    }
    
    monitoring_ = false;
    if (monitor_thread_.joinable()) {
        monitor_thread_.join();
    }
    Logger::instance().info("FailoverManager stopped monitoring");
}

bool FailoverManager::promoteSlaveToMaster(const std::string& slave_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 查找slave节点
    auto slave_it = cluster_nodes_.find(slave_id);
    if (slave_it == cluster_nodes_.end()) {
        Logger::instance().error("Slave " + slave_id + " not found in cluster");
        return false;
    }
    
    if (slave_it->second.role != "slave") {
        Logger::instance().error("Node " + slave_id + " is not a slave (role: " + 
                                slave_it->second.role + ")");
        return false;
    }
    
    // 查找当前master
    std::string old_master_id;
    for (auto& pair : cluster_nodes_) {
        if (pair.second.role == "master") {
            old_master_id = pair.first;
            pair.second.role = "slave";  // 将原master降级为slave
            break;
        }
    }
    
    if (!old_master_id.empty()) {
        Logger::instance().info("Demoted old master " + old_master_id + " to slave");
    }
    
    // 将slave升级为master
    slave_it->second.role = "master";
    Logger::instance().info("Promoted slave " + slave_id + " to master");
    
    return true;
}

bool FailoverManager::demoteMasterToSlave(const std::string& master_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 查找master节点
    auto master_it = cluster_nodes_.find(master_id);
    if (master_it == cluster_nodes_.end()) {
        Logger::instance().error("Master " + master_id + " not found in cluster");
        return false;
    }
    
    if (master_it->second.role != "master") {
        Logger::instance().error("Node " + master_id + " is not a master (role: " + 
                                master_it->second.role + ")");
        return false;
    }
    
    master_it->second.role = "slave";
    Logger::instance().info("Demoted master " + master_id + " to slave");
    
    return true;
}

std::string FailoverManager::getMasterId() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (const auto& pair : cluster_nodes_) {
        if (pair.second.role == "master") {
            return pair.first;
        }
    }
    return "";
}

std::vector<std::string> FailoverManager::getSlaveIds() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> slaves;
    for (const auto& pair : cluster_nodes_) {
        if (pair.second.role == "slave") {
            slaves.push_back(pair.first);
        }
    }
    return slaves;
}

std::string FailoverManager::getClusterStatus() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string status = "Cluster status:\n";
    for (const auto& pair : cluster_nodes_) {
        status += "  Node " + pair.first + ": " + pair.second.role + 
                 " [" + (pair.second.is_alive ? "alive" : "dead") + "]\n";
    }
    return status;
}

void FailoverManager::monitorThreadFunc() {
    Logger::instance().info("Cluster monitor thread started");
    
    while (monitoring_) {
        try {
            detectMasterFailure();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        } catch (const std::exception& e) {
            Logger::instance().error(std::string("Error in monitor thread: ") + e.what());
        }
    }
    
    Logger::instance().info("Cluster monitor thread stopped");
}

void FailoverManager::detectMasterFailure() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string master_id = getMasterId();
    if (master_id.empty()) {
        Logger::instance().warning("No master found in cluster");
        return;
    }
    
    auto master_it = cluster_nodes_.find(master_id);
    if (master_it != cluster_nodes_.end() && !master_it->second.is_alive) {
        Logger::instance().warning("Master " + master_id + " is dead, initiating failover...");
        
        // 选择新的master
        ClusterNode new_master = selectNewMaster();
        if (new_master.id.empty()) {
            Logger::instance().error("No suitable slave found for failover");
            return;
        }
        
        // 更新角色
        master_it->second.role = "slave";  // 原master降级为slave
        cluster_nodes_[new_master.id].role = "master";
        
        Logger::instance().info("Failover completed: " + new_master.id + " is new master");
    }
}

ClusterNode FailoverManager::selectNewMaster() {
    ClusterNode best_candidate;
    int highest_priority = -1;
    
    for (const auto& pair : cluster_nodes_) {
        if (pair.second.role == "slave" && pair.second.is_alive) {
            if (pair.second.priority > highest_priority) {
                highest_priority = pair.second.priority;
                best_candidate = pair.second;
            }
        }
    }
    
    return best_candidate;
}

bool FailoverManager::initiateElection() {
    Logger::instance().info("Initiating election for new master...");
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 检查当前节点是否有资格参与选举
    auto current_it = cluster_nodes_.find(current_node_id_);
    if (current_it == cluster_nodes_.end()) {
        Logger::instance().error("Current node " + current_node_id_ + 
                                " cannot participate in election (not in cluster)");
        return false;
    }
    
    Logger::instance().info("Node " + current_node_id_ + " is requesting votes...");
    
    // 统计存活节点
    int total_alive = 0;
    for (const auto& pair : cluster_nodes_) {
        if (pair.second.is_alive) {
            total_alive++;
        }
    }
    
    // 简单选举：当前节点获得所有存活节点的投票
    int votes = total_alive;  // 假设所有存活节点都投票给当前节点
    
    if (votes > total_alive / 2) {
        // 选举成功
        for (auto& pair : cluster_nodes_) {
            if (pair.second.role == "master") {
                pair.second.role = "slave";  // 降级原master
            }
        }
        
        current_it->second.role = "master";  // 当前节点成为新master
        current_it->second.is_alive = true;
        
        Logger::instance().info("Election successful! Node " + current_node_id_ + 
                               " is now master with " + std::to_string(votes) + 
                               "/" + std::to_string(total_alive) + " votes");
        return true;
    } else {
        Logger::instance().info("Election failed: only " + std::to_string(votes) + 
                               "/" + std::to_string(total_alive) + " votes");
        return false;
    }
}
