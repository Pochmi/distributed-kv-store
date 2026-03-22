#!/bin/bash
# tests/phase3/quick_test.sh - 简化版

echo "=== 阶段三测试：主从复制 ==="

cd ~/桌面/distributed-kv-store

# 清理
pkill -f kv_server 2>/dev/null
sleep 1

# 启动主节点
echo "启动主节点..."
./build/bin/kv_server --port 6381 --role master --replication > /tmp/master.log 2>&1 &
MASTER=$!
sleep 2

# 写入测试数据
echo "写入数据到主节点..."
./build/bin/kv_client set name "张三" 2>&1 | grep "成功"

# 启动从节点
echo "启动从节点..."
./build/bin/kv_server --port 6382 --role slave --master 127.0.0.1:6380 --replication > /tmp/slave.log 2>&1 &
SLAVE=$!
sleep 3

# 验证复制
echo "验证从节点数据..."
RESULT=$(./build/bin/kv_client get name 2>&1)
if echo "$RESULT" | grep -q "张三"; then
    echo "✅ 主从复制成功！从节点读到: $RESULT"
else
    echo "❌ 复制失败"
fi

# 清理
kill $MASTER $SLAVE 2>/dev/null
echo "测试完成"