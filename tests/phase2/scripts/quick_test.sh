#!/bin/bash
# 阶段二快速测试：分布式分片功能

echo "=== 分布式KV存储 - 快速测试 ==="
echo "测试分片功能和客户端路由"
echo ""

cd ~/桌面/distributed-kv-store

# 1. 编译检查
echo "1. 检查编译状态..."
if [ ! -f "build/kv_server" ] || [ ! -f "build/kv_client" ]; then
    echo "   重新编译项目..."
    ./scripts/build.sh
fi

# 2. 清理环境
echo "2. 清理旧进程..."
pkill -f "kv_server" 2>/dev/null || true
sleep 2

# 3. 启动3个分片服务器
echo "3. 启动分布式集群..."
PORTS=(6381 6382 6383)
SERVER_PIDS=()
SERVER_LOGS=()

for idx in "${!PORTS[@]}"; do
    PORT=${PORTS[$idx]}
    NODE_ID=$((idx + 1))
    LOG_FILE="/tmp/kv_server_${PORT}.log"
    
    echo "   启动节点${NODE_ID} (端口: ${PORT})..."
    ./build/kv_server $PORT > $LOG_FILE 2>&1 &
    SERVER_PIDS+=($!)
    SERVER_LOGS+=($LOG_FILE)
    sleep 1
    
    if kill -0 ${SERVER_PIDS[$idx]} 2>/dev/null; then
        echo "   ✅ 成功"
    else
        echo "   ❌ 失败"
        cat $LOG_FILE
        exit 1
    fi
done

echo "✅ 3个节点启动完成"
sleep 2

# 4. 快速功能测试
echo ""
echo "4. 运行快速功能测试..."

echo "🔹 测试1: 基本PUT操作"
echo "-------------------"
for i in {1..3}; do
    echo "   PUT key${i} value${i}"
    ./build/kv_client set key${i} value${i} 2>&1 | grep -E "成功|Node|节点|server"
done

echo ""
echo "🔹 测试2: 基本GET操作"
echo "-------------------"
for i in {1..3}; do
    echo "   GET key${i}"
    ./build/kv_client get key${i} 2>&1 | grep -E "value|OK|成功"
done

echo ""
echo "🔹 测试3: 分片路由测试"
echo "---------------------"
echo "   观察不同key是否路由到不同节点:"
for i in {1..5}; do
    KEY="item_${i}"
    echo -n "   ${KEY}: "
    ./build/kv_client set ${KEY} "test${i}" 2>&1 | grep -o "Node.*server-[0-9]\|->.*:" | head -1
done

echo ""
echo "🔹 测试4: 删除功能"
echo "-----------------"
echo "   DELETE key1"
./build/kv_client del key1 2>&1 | grep -E "成功|SUCCESS|OK"
echo "   GET key1 (应不存在)"
./build/kv_client get key1 2>&1 | grep -E "不存在|not found|failed"

# 5. 停止服务器
echo ""
echo "5. 停止服务器..."
for PID in "${SERVER_PIDS[@]}"; do
    kill $PID 2>/dev/null || true
done

sleep 1

# 6. 检查各服务器日志
echo ""
echo "6. 服务器日志统计:"
for idx in "${!PORTS[@]}"; do
    PORT=${PORTS[$idx]}
    LOG_FILE="/tmp/kv_server_${PORT}.log"
    if [ -f "$LOG_FILE" ]; then
        LINE_COUNT=$(wc -l < "$LOG_FILE")
        echo "   节点${PORT}: ${LINE_COUNT} 行日志"
    fi
done

# 7. 清理临时文件
for LOG_FILE in "${SERVER_LOGS[@]}"; do
    rm -f "$LOG_FILE" 2>/dev/null || true
done

echo ""
echo "✅ 快速测试完成！"
echo ""
echo "💡 提示："
echo "1. 如果看到'Node server-X'或'-> 127.0.0.1:端口'，说明分片工作正常"
echo "2. 不同key应该被路由到不同服务器节点"
echo "3. 这是快速测试，详细测试请运行 run_all.sh"
