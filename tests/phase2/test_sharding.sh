#!/bin/bash
# tests/phase2/test_sharding.sh - 阶段二分布式分片测试

echo "=========================================="
echo "  阶段二测试：分布式分片"
echo "=========================================="
echo ""

# 获取项目目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
cd "$PROJECT_DIR"

# 清理旧进程
echo "1. 清理旧进程..."
pkill -f kv_server 2>/dev/null
sleep 1

# 检查编译
if [ ! -f "build/bin/kv_server" ]; then
    echo "错误: 请先编译项目"
    exit 1
fi

# 启动3个分片节点
echo "2. 启动3个分片节点..."
echo "启动节点1 (端口 6381)..."
./build/bin/kv_server --port 6381 --cluster > /tmp/shard1.log 2>&1 &
PID1=$!

echo "启动节点2 (端口 6382)..."
./build/bin/kv_server --port 6382 --cluster > /tmp/shard2.log 2>&1 &
PID2=$!

echo "启动节点3 (端口 6383)..."
./build/bin/kv_server --port 6383 --cluster > /tmp/shard3.log 2>&1 &
PID3=$!

sleep 3

# 检查节点是否运行
for pid in $PID1 $PID2 $PID3; do
    if ! kill -0 $pid 2>/dev/null; then
        echo "错误: 节点启动失败"
        kill $PID1 $PID2 $PID3 2>/dev/null
        exit 1
    fi
done
echo "✅ 3个节点启动成功"

# 写入测试数据
echo ""
echo "3. 写入20个测试键值对..."
for i in {1..20}; do
    KEY="test_key_$i"
    VALUE="value_$i"
    ./build/bin/kv_client set "$KEY" "$VALUE" > /dev/null 2>&1
    echo -n "."
done
echo " 完成"

# 验证数据可读取
echo ""
echo "4. 验证数据读取..."
SUCCESS=0
FAIL=0
for i in {1..20}; do
    KEY="test_key_$i"
    EXPECTED="value_$i"
    RESULT=$(./build/bin/kv_client get "$KEY" 2>&1)
    if echo "$RESULT" | grep -q "$EXPECTED"; then
        ((SUCCESS++))
    else
        echo "❌ $KEY 读取失败"
        ((FAIL++))
    fi
done

echo ""
echo "5. 测试结果:"
echo "   成功: $SUCCESS/20"
echo "   失败: $FAIL/20"

# 查看路由分布
echo ""
echo "6. 查看各节点处理的键数量:"
for port in 6381 6382 6383; do
    count=$(grep -c "Processing.*test_key" /tmp/shard$port.log 2>/dev/null || echo 0)
    echo "   节点 $port: $count 个键"
done

# 清理
echo ""
echo "7. 清理测试环境..."
kill $PID1 $PID2 $PID3 2>/dev/null
sleep 1

echo ""
if [ $FAIL -eq 0 ]; then
    echo "✅ 阶段二测试通过！"
else
    echo "❌ 阶段二测试失败"
fi