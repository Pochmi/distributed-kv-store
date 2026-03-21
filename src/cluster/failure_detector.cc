#include "failure_detector.h"
#include "../common/logger.h"
#include <chrono>
#include <thread>

FailureDetector::FailureDetector(HeartbeatManager* heartbeat_mgr) 
    : heartbeat_mgr_(heartbeat_mgr), running_(false) {
    
    if (!heartbeat_mgr_) {
        Logger::instance().error("FailureDetector: HeartbeatManager is null");
        return;
    }
    
    Logger::instance().info("FailureDetector initialized");
}

FailureDetector::~FailureDetector() {
    stop();
}

void FailureDetector::setFailureCallback(FailureCallback callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    failure_callback_ = callback;
    Logger::instance().debug("Failure callback set");
}

void FailureDetector::start() {
    if (running_) {
        Logger::instance().warning("FailureDetector already running");
        return;
    }
    
    running_ = true;
    detection_thread_ = std::thread(&FailureDetector::detectionThreadFunc, this);
    Logger::instance().info("FailureDetector started");
}

void FailureDetector::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    if (detection_thread_.joinable()) {
        detection_thread_.join();
    }
    Logger::instance().info("FailureDetector stopped");
}

void FailureDetector::detectionThreadFunc() {
    Logger::instance().info("Failure detection thread started");
    
    while (running_) {
        try {
            if (!heartbeat_mgr_) {
                Logger::instance().error("HeartbeatManager is null, stopping detection");
                break;
            }
            
            // 获取死亡的节点
            auto dead_nodes = heartbeat_mgr_->getDeadNodes();
            
            // 对每个死亡的节点触发回调
            for (const auto& node_id : dead_nodes) {
                if (!running_) break;
                
                Logger::instance().warning("Node " + node_id + " detected as failed");
                
                std::lock_guard<std::mutex> lock(callback_mutex_);
                if (failure_callback_) {
                    try {
                        failure_callback_(node_id);
                    } catch (const std::exception& e) {
                        Logger::instance().error(std::string("Failure callback error: ") + e.what());
                    }
                }
            }
            
            // 每秒检查一次
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
        } catch (const std::exception& e) {
            Logger::instance().error(std::string("Error in failure detection thread: ") + e.what());
        }
    }
    
    Logger::instance().info("Failure detection thread stopped");
}

std::vector<std::string> FailureDetector::getFailedNodes() const {
    if (!heartbeat_mgr_) {
        return std::vector<std::string>();
    }
    return heartbeat_mgr_->getDeadNodes();
}
