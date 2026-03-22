#!/bin/bash
# tests/phase2/check_distribution.sh - 检查分片分布情况

echo "=== 检查分片分布 ==="

cd ~/桌面/distributed-kv-store

# 清理
pkill -f kv_server 2>/dev/null
sleep 1

# 启动3个节点
./build/bin/kv_server --port 6381 --cluster > /tmp/shard1.log 2>&1 &
./build/bin/kv_server --port 6382 --cluster > /tmp/shard2.log 2>&1 &
./build/bin/kv_server --port 6383 --cluster > /tmp/shard3.log 2>&1 &
sleep 3

# 写入100个键
echo "写入100个测试键..."
for i in {1..100}; do
    ./build/bin/kv_client set "dist_key_$i" "data_$i" > /dev/null 2>&1
    if [ $((i % 10)) -eq 0 ]; then
        echo -n "."
    fi
done
echo " 完成"

# 统计分布
echo ""
echo "分片分布统计:"
for port in 6381 6382 6383; do
    count=$(grep -c "SET dist_key_" /tmp/shard$port.log 2>/dev/null || echo 0)
    echo "  节点 $port: $count 个键"
done

# 清理
pkill -f kv_server 2>/dev/null