// src/core/kv_store.h
#ifndef KV_STORE_H
#define KV_STORE_H

#include <string>
#include <memory>

enum StatusCode {
    OK = 0,
    KEY_NOT_FOUND = 1,
    STORAGE_ERROR = 2,
    INVALID_ARGUMENT = 3
};

struct Status {
    StatusCode code;
    std::string message;
    
    Status() : code(OK) {}
    Status(StatusCode c, const std::string& msg = "") : code(c), message(msg) {}
    
    bool ok() const { return code == OK; }
    bool is_key_not_found() const { return code == KEY_NOT_FOUND; }
    bool is_error() const { return code != OK; }
    
    static Status OK_STATUS() { return Status(); }
    static Status KeyNotFound(const std::string& key) {
        return Status(KEY_NOT_FOUND, "Key not found: " + key);
    }
    static Status Error(const std::string& msg) {
        return Status(STORAGE_ERROR, msg);
    }
};

class KVStore {
public:
    virtual ~KVStore() = default;
    
    virtual Status Put(const std::string& key, const std::string& value) = 0;
    virtual Status Get(const std::string& key, std::string& value) = 0;
    virtual Status Delete(const std::string& key) = 0;
    virtual Status Contains(const std::string& key) = 0;
    virtual size_t Size() const = 0;
    virtual void Clear() = 0;
    
    // 工厂方法：创建内存存储实例
    static std::unique_ptr<KVStore> CreateMemoryStore();
};

#endif // KV_STORE_H
