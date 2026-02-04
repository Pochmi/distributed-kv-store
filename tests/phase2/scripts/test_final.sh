#!/bin/bash

echo "=== 最终功能测试 ==="
echo "时间: $(date)"
echo ""

cd ~/桌面/distributed-kv-store

# 1. 清理环境
echo "1. 清理环境..."
pkill -f "kv_server" 2>/dev/null
sleep 1

# 2. 编译检查
echo "2. 编译检查..."
cd build
make kv_client kv_server 2>&1 | tail -3
cd ..

# 3. 启动服务器
echo "3. 启动服务器..."
./build/kv_server 6381 "final-test-server" > /tmp/kv_test.log 2>&1 &
SERVER_PID=$!
sleep 3

if ! ps -p $SERVER_PID > /dev/null; then
    echo "❌ 服务器启动失败"
    cat /tmp/kv_test.log
    exit 1
fi
echo "✅ 服务器启动成功 (PID: $SERVER_PID)"

# 4. 运行测试
echo "4. 运行测试..."
echo ""
echo "测试用例1: SET/GET/DEL 流程"
echo "--------------------------"

echo "SET key1 = value1"
if ./build/kv_client set key1 "value1" 2>&1 | grep -q "成功"; then
    echo "  ✅ SET 成功"
else
    echo "  ❌ SET 失败"
    exit 1
fi

echo ""
echo "GET key1"
RESULT=$(./build/kv_client get key1 2>&1)
if echo "$RESULT" | grep -q "value1"; then
    echo "  ✅ GET 成功: $RESULT"
else
    echo "  ❌ GET 失败: $RESULT"
    exit 1
fi

echo ""
echo "DEL key1"
if ./build/kv_client del key1 2>&1 | grep -q "成功"; then
    echo "  ✅ DEL 成功"
else
    echo "  ❌ DEL 失败"
    exit 1
fi

echo ""
echo "验证删除"
RESULT=$(./build/kv_client get key1 2>&1)
if echo "$RESULT" | grep -q "不存在\|not found"; then
    echo "  ✅ 验证成功: 键已删除"
else
    echo "  ❌ 验证失败: $RESULT"
    exit 1
fi

# 5. 清理
echo ""
echo "5. 清理..."
kill $SERVER_PID 2>/dev/null
sleep 1

echo ""
echo "🎉 所有测试通过！"
echo "阶段二功能验证完成。"
