// tests/unit/test_kv_store.cc
#include "src/core/kv_store.h"
#include <gtest/gtest.h>
#include <memory>

class MemoryStoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        store = KVStore::CreateMemoryStore();
    }
    
    std::unique_ptr<KVStore> store;
};

TEST_F(MemoryStoreTest, BasicPutGet) {
    std::string value;
    
    // 测试PUT
    EXPECT_TRUE(store->Put("key1", "value1").ok());
    EXPECT_TRUE(store->Put("key2", "value2").ok());
    
    // 测试GET
    EXPECT_TRUE(store->Get("key1", value).ok());
    EXPECT_EQ(value, "value1");
    
    EXPECT_TRUE(store->Get("key2", value).ok());
    EXPECT_EQ(value, "value2");
}

TEST_F(MemoryStoreTest, KeyNotFound) {
    std::string value;
    
    Status status = store->Get("nonexistent", value);
    EXPECT_TRUE(status.is_key_not_found());
}

TEST_F(MemoryStoreTest, Delete) {
    std::string value;
    
    store->Put("key1", "value1");
    EXPECT_TRUE(store->Delete("key1").ok());
    
    Status status = store->Get("key1", value);
    EXPECT_TRUE(status.is_key_not_found());
}

TEST_F(MemoryStoreTest, Size) {
    EXPECT_EQ(store->Size(), 0);
    
    store->Put("key1", "value1");
    EXPECT_EQ(store->Size(), 1);
    
    store->Put("key2", "value2");
    EXPECT_EQ(store->Size(), 2);
    
    store->Delete("key1");
    EXPECT_EQ(store->Size(), 1);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
