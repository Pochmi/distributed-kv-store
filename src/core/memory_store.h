// src/core/memory_store.h
#ifndef MEMORY_STORE_H
#define MEMORY_STORE_H

#include "kv_store.h"
#include <unordered_map>
#include <mutex>

class MemoryStore : public KVStore {
public:
    Status Put(const std::string& key, const std::string& value) override;
    Status Get(const std::string& key, std::string& value) override;
    Status Delete(const std::string& key) override;
    Status Contains(const std::string& key) override;
    size_t Size() const override;
    void Clear() override;

private:
    std::unordered_map<std::string, std::string> data_;
    mutable std::mutex mutex_;
};

#endif // MEMORY_STORE_H
