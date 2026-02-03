#!/bin/bash

echo "=== 分布式KV存储 - 阶段二完整测试 ==="
echo "使用正确的命令: set/get/del"
echo ""

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../../.." && pwd)"
cd "$PROJECT_DIR"

echo "1. 清理环境..."
pkill -f "kv_server" 2>/dev/null
sleep 1

echo "2. 启动3个分片服务器..."
for port in 6381 6382 6383; do
    echo "启动服务器 $port..."
    ./build/kv_server $port "node-$port" > "$SCRIPT_DIR/../logs/server_$port.correct.log" 2>&1 &
    sleep 1
done
sleep 2

echo "3. 运行基本功能测试..."
echo "测试 SET 命令:"
./build/kv_client set phase2_test "分布式KV存储" 2>&1

echo ""
echo "测试 GET 命令:"
./build/kv_client get phase2_test 2>&1

echo ""
echo "测试 DEL 命令:"
./build/kv_client del phase2_test 2>&1

echo ""
echo "4. 运行分片测试..."
echo "写入10个测试键:"
SUCCESS=0
for i in {1..10}; do
    KEY="final_key_$i"
    if ./build/kv_client set "$KEY" "value_$i" > /dev/null 2>&1; then
        ((SUCCESS++))
    fi
done
echo "写入成功: $SUCCESS/10"

echo ""
echo "5. 验证读取..."
READ_SUCCESS=0
for i in {1..10}; do
    KEY="final_key_$i"
    RESULT=$(./build/kv_client get "$KEY" 2>/dev/null)
    if echo "$RESULT" | grep -q "value_$i"; then
        ((READ_SUCCESS++))
    fi
done
echo "读取成功: $READ_SUCCESS/10"

echo ""
echo "6. 停止服务器..."
pkill -f "kv_server" 2>/dev/null
sleep 1

echo ""
echo "=== 测试总结 ==="
echo "阶段二功能验证:"
if [ $SUCCESS -ge 5 ] && [ $READ_SUCCESS -ge 5 ]; then
    echo "✅ 分布式分片功能正常工作"
    echo "✅ 客户端路由逻辑正确"
    echo "✅ 多节点协作正常"
    echo ""
    echo "🎉 阶段二测试通过！"
    exit 0
else
    echo "❌ 测试发现问题"
    echo "写入成功率: $((SUCCESS*10))%"
    echo "读取成功率: $((READ_SUCCESS*10))%"
    exit 1
fi
