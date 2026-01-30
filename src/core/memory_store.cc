// src/core/memory_store.cc
#include "memory_store.h"
#include "../common/logger.h"

Status MemoryStore::Put(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (key.empty()) {
        return Status::Error("Key cannot be empty");
    }
    
    data_[key] = value;
    LOG_DEBUG("Put key: " + key + ", value: " + value);
    return Status::OK_STATUS();
}

Status MemoryStore::Get(const std::string& key, std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = data_.find(key);
    if (it == data_.end()) {
        LOG_DEBUG("Key not found: " + key);
        return Status::KeyNotFound(key);
    }
    
    value = it->second;
    LOG_DEBUG("Get key: " + key + ", value: " + value);
    return Status::OK_STATUS();
}

Status MemoryStore::Delete(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (data_.erase(key) == 0) {
        return Status::KeyNotFound(key);
    }
    
    LOG_DEBUG("Delete key: " + key);
    return Status::OK_STATUS();
}

Status MemoryStore::Contains(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_.find(key) != data_.end() ? Status::OK_STATUS() : Status::KeyNotFound(key);
}

size_t MemoryStore::Size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_.size();
}

void MemoryStore::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    data_.clear();
    LOG_INFO("Memory store cleared");
}

// 工厂方法实现
std::unique_ptr<KVStore> KVStore::CreateMemoryStore() {
    return std::make_unique<MemoryStore>();
}
