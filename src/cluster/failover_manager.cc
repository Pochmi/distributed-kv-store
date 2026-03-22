#include "failover_manager.h"
#include "../common/logger.h"

FailoverManager::FailoverManager(HeartbeatManager* heartbeat)
    : heartbeat_(heartbeat) {
    LOG_INFO("FailoverManager initialized");
}

FailoverManager::~FailoverManager() = default;

void FailoverManager::setMaster(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (current_master_ != node_id) {
        std::string old = current_master_;
        current_master_ = node_id;
        LOG_INFO("Master changed: " + old + " -> " + current_master_);
        if (callback_) {
            callback_(old, current_master_);
        }
    }
}

std::string FailoverManager::getMaster() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_master_;
}

void FailoverManager::addSlave(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    slaves_.push_back(node_id);
    LOG_INFO("Added slave: " + node_id);
}

void FailoverManager::checkAndFailover() {
    if (current_master_.empty()) return;
    
    if (!heartbeat_->isAlive(current_master_)) {
        LOG_WARNING("Master " + current_master_ + " is down, initiating failover...");
        
        std::lock_guard<std::mutex> lock(mutex_);
        // 选择第一个存活的从节点作为新主
        for (const auto& slave : slaves_) {
            if (heartbeat_->isAlive(slave)) {
                std::string old = current_master_;
                current_master_ = slave;
                LOG_INFO("Failover complete: " + old + " -> " + current_master_);
                if (callback_) {
                    callback_(old, current_master_);
                }
                return;
            }
        }
        LOG_ERROR("No alive slave found for failover!");
    }
}

bool FailoverManager::manualFailover(const std::string& new_master) {
    if (new_master == current_master_) return true;
    
    if (!heartbeat_->isAlive(new_master)) {
        LOG_ERROR("Cannot failover to " + new_master + " (node is down)");
        return false;
    }
    
    setMaster(new_master);
    return true;
}

void FailoverManager::setFailoverCallback(Callback cb) {
    callback_ = cb;
}