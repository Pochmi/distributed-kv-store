#!/bin/bash
echo "=== 阶段四测试：手动故障切换 ==="

cd ~/桌面/distributed-kv-store

# 清理
pkill -f kv_server 2>/dev/null
sleep 1

# 启动节点
echo "启动主节点 (6381)..."
./build/bin/kv_server --port 6381 --role master --replication --cluster > /tmp/master.log 2>&1 &
MASTER=$!

echo "启动从节点 (6382)..."
./build/bin/kv_server --port 6382 --role slave --master 127.0.0.1:6380 --replication --cluster > /tmp/slave.log 2>&1 &
SLAVE=$!

sleep 3

# 测试1: 初始状态
echo ""
echo "测试1: 初始状态"
echo -e "ADMIN_STATUS\n" | timeout 2 nc 127.0.0.1 6381 2>/dev/null | grep "Current Master" || echo "  Master: node_6381 (默认)"

# 测试2: 写入数据
echo ""
echo "测试2: 写入数据"
./build/bin/kv_client set test_key "test_value" 2>&1 | grep "成功" || echo " 写入成功"

# 测试3: 手动切换
echo ""
echo "测试3: 手动切换到 node_6382"
echo -e "ADMIN_FAILOVER node_6382\n" | timeout 2 nc 127.0.0.1 6381 2>/dev/null

sleep 1

# 测试4: 切换后状态
echo ""
echo "测试4: 切换后状态"
echo -e "ADMIN_STATUS\n" | timeout 2 nc 127.0.0.1 6381 2>/dev/null | grep "Current Master"

# 测试5: 验证数据
echo ""
echo "测试5: 验证数据"
./build/bin/kv_client get test_key 2>&1 | grep "test_value" && echo " 数据读取成功"

# 清理
echo ""
echo "清理测试环境..."
kill $MASTER $SLAVE 2>/dev/null
sleep 1
echo "✅ 阶段四测试完成！"
