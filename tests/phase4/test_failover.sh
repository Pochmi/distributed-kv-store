#!/bin/bash
echo "=== 阶段四测试：故障切换 ==="

cd ~/桌面/distributed-kv-store

# 清理
pkill -f kv_server 2>/dev/null
sleep 1

# 启动节点
echo "启动节点1 (主节点 6381)..."
./build/bin/kv_server --port 6381 --role master --replication --cluster > /tmp/node1.log 2>&1 &
PID1=$!

echo "启动节点2 (从节点 6382)..."
./build/bin/kv_server --port 6382 --role slave --master 127.0.0.1:6380 --replication --cluster > /tmp/node2.log 2>&1 &
PID2=$!

sleep 3

# 写入数据（使用会路由到主节点 6381 的 key，如 "name"）
echo "写入测试数据到主节点..."
./build/bin/kv_client set name "张三" 2>&1 | grep -E "成功|失败"

# 验证从节点有数据
sleep 1
echo "验证从节点数据..."
RESULT=$(./build/bin/kv_client get name 2>&1)
if echo "$RESULT" | grep -q "张三"; then
    echo "✅ 数据已复制到从节点"
else
    echo "❌ 数据未复制"
fi

# 模拟主节点故障
echo ""
echo "模拟主节点故障 (kill PID: $PID1)..."
kill $PID1
sleep 3

# 检查数据是否仍然可用
echo ""
echo "检查故障切换后数据是否可用..."
RESULT=$(./build/bin/kv_client get name 2>&1)
if echo "$RESULT" | grep -q "张三"; then
    echo "✅ 故障切换成功！数据仍然可读"
else
    echo "❌ 故障切换失败: $RESULT"
fi

# 清理
kill $PID2 2>/dev/null
echo ""
echo "测试完成"
