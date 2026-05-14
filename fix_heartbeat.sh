#!/bin/bash
sed -i 's/std::lock_guard<std::mutex> lock(mutex_);/std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mutex_));/' src/cluster/heartbeat.cc
