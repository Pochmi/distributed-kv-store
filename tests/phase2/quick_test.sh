#!/bin/bash
# tests/phase2/quick_test.sh - 阶段二快速测试

echo "=== 阶段二测试：分布式分片 ==="

cd ~/桌面/distributed-kv-store

# 清理
pkill -f kv_server 2>/dev/null
sleep 1

# 启动3个节点
echo "启动3个分片节点..."
./build/bin/kv_server --port 6381 --cluster > /tmp/shard1.log 2>&1 &
./build/bin/kv_server --port 6382 --cluster > /tmp/shard2.log 2>&1 &
./build/bin/kv_server --port 6383 --cluster > /tmp/shard3.log 2>&1 &
sleep 3

# 写入测试数据
echo "写入10个测试键..."
for i in {1..10}; do
    ./build/bin/kv_client set "key_$i" "value_$i" 2>&1 | grep -E "成功|失败"
done

# 读取验证
echo ""
echo "读取验证..."
SUCCESS=0
for i in {1..10}; do
    RESULT=$(./build/bin/kv_client get "key_$i" 2>&1)
    if echo "$RESULT" | grep -q "value_$i"; then
        ((SUCCESS++))
    fi
done
echo "成功读取: $SUCCESS/10"

# 查看路由分布
echo ""
echo "路由分布:"
for port in 6381 6382 6383; do
    count=$(grep -c "SET key_" /tmp/shard$port.log 2>/dev/null || echo 0)
    echo "  节点 $port: $count 个键"
done

# 清理
pkill -f kv_server 2>/dev/null

if [ $SUCCESS -eq 10 ]; then
    echo "✅ 阶段二测试通过！"
else
    echo "❌ 阶段二测试失败"
fi