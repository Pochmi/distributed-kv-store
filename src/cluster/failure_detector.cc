#include "failure_detector.h"
#include "common/logger.h"

#include <thread>
#include <chrono>

FailureDetector::FailureDetector(HeartbeatManager* heartbeat_mgr)
    : heartbeat_mgr_(heartbeat_mgr)
    , running_(false) {
    
    if (!heartbeat_mgr_) {
        Logger::error("FailureDetector: HeartbeatManager is null");
        throw std::invalid_argument("HeartbeatManager cannot be null");
    }
    
    Logger::info("FailureDetector initialized");
}

FailureDetector::~FailureDetector() {
    stop();
}

void FailureDetector::setFailureCallback(FailureCallback callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    failure_callback_ = callback;
    Logger::debug("Failure callback set");
}

void FailureDetector::start() {
    if (running_) {
        Logger::warn("FailureDetector already running");
        return;
    }
    
    running_ = true;
    detection_thread_ = std::thread(&FailureDetector::detectionThreadFunc, this);
    Logger::info("FailureDetector started");
}

void FailureDetector::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    
    if (detection_thread_.joinable()) {
        detection_thread_.join();
    }
    
    Logger::info("FailureDetector stopped");
}

std::vector<std::string> FailureDetector::getFailedNodes() const {
    if (!heartbeat_mgr_) {
        return {};
    }
    
    return heartbeat_mgr_->getDeadNodes();
}

void FailureDetector::detectionThreadFunc() {
    Logger::info("Failure detection thread started");
    
    // 存储已通知的故障节点，避免重复通知
    std::vector<std::string> notified_failures;
    
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));  // 每2秒检查一次
        
        try {
            if (!heartbeat_mgr_) {
                Logger::error("HeartbeatManager is null, stopping detection");
                break;
            }
            
            // 获取死亡节点列表
            auto dead_nodes = heartbeat_mgr_->getDeadNodes();
            
            // 检查是否有新的故障节点
            for (const auto& node_id : dead_nodes) {
                // 检查是否已经通知过
                if (std::find(notified_failures.begin(), notified_failures.end(), node_id) 
                    == notified_failures.end()) {
                    
                    Logger::warn("Detected node failure: %s", node_id.c_str());
                    
                    // 调用回调函数
                    {
                        std::lock_guard<std::mutex> lock(callback_mutex_);
                        if (failure_callback_) {
                            try {
                                failure_callback_(node_id);
                            } catch (const std::exception& e) {
                                Logger::error("Failure callback error: %s", e.what());
                            }
                        }
                    }
                    
                    // 记录已通知
                    notified_failures.push_back(node_id);
                }
            }
            
            // 清理已恢复的节点（可选）
            // 如果节点恢复，可以从notified_failures中移除
            
        } catch (const std::exception& e) {
            Logger::error("Error in failure detection thread: %s", e.what());
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
    
    Logger::info("Failure detection thread stopped");
}
