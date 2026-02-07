#ifndef FAILURE_DETECTOR_H
#define FAILURE_DETECTOR_H

#include "heartbeat.h"
#include <functional>
#include <vector>

class FailureDetector {
public:
    using FailureCallback = std::function<void(const std::string& node_id)>;
    
    FailureDetector(HeartbeatManager* heartbeat_mgr);
    ~FailureDetector();
    
    void setFailureCallback(FailureCallback callback);
    void start();
    void stop();
    
    std::vector<std::string> getFailedNodes() const;
    
private:
    void detectionThreadFunc();
    
    HeartbeatManager* heartbeat_mgr_;
    FailureCallback failure_callback_;
    
    std::thread detection_thread_;
    std::atomic<bool> running_;
    mutable std::mutex callback_mutex_;
};

#endif // FAILURE_DETECTOR_H
