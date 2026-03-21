#!/bin/bash
# tests/phase3/quick_test.sh - 阶段三快速测试（主从复制）

echo "=== 阶段三快速测试：主从复制 ==="

# 获取项目根目录的绝对路径
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo "项目目录: $PROJECT_DIR"
cd "$PROJECT_DIR"

# 清理之前的进程
echo "清理旧进程..."
pkill -f kv_server 2>/dev/null || echo "没有旧进程需要清理"
sleep 1

# 检查是否已编译
if [ ! -f "build/kv_server" ]; then
    echo "错误: 请先运行 ./scripts/build.sh 编译项目"
    exit 1
fi

# 启动主节点
echo "1. 启动主节点 (端口 6380)..."
./build/kv_server --port 6380 --role master > /tmp/kv_master_phase3.log 2>&1 &
MASTER_PID=$!
echo "主节点 PID: $MASTER_PID"

# 等待主节点启动
echo "等待主节点启动..."
sleep 3

# 检查主节点是否运行
if ! kill -0 $MASTER_PID 2>/dev/null; then
    echo "错误: 主节点启动失败"
    echo "查看日志: /tmp/kv_master_phase3.log"
    tail -20 /tmp/kv_master_phase3.log
    exit 1
fi

# 写入测试数据到主节点
echo "2. 写入测试数据到主节点..."
echo "写入: SET name zhangsan"
./build/kv_client --host 127.0.0.1 --port 6380 <<< "SET name zhangsan" 2>/dev/null

echo "写入: SET age 25"
./build/kv_client --host 127.0.0.1 --port 6380 <<< "SET age 25" 2>/dev/null

# 启动从节点
echo "3. 启动从节点 (端口 6381)..."
./build/kv_server --port 6381 --role slave --master 127.0.0.1:6380 > /tmp/kv_slave_phase3.log 2>&1 &
SLAVE_PID=$!
echo "从节点 PID: $SLAVE_PID"

# 等待从节点连接和同步
echo "等待从节点连接和同步 (3秒)..."
sleep 3

# 检查从节点是否运行
if ! kill -0 $SLAVE_PID 2>/dev/null; then
    echo "错误: 从节点启动失败"
    echo "查看日志: /tmp/kv_slave_phase3.log"
    tail -20 /tmp/kv_slave_phase3.log
    kill $MASTER_PID 2>/dev/null
    exit 1
fi

# 从从节点读取数据验证复制
echo "4. 验证数据复制..."
echo "从从节点读取 'name':"
RESULT=$(./build/kv_client --host 127.0.0.1 --port 6381 <<< "GET name" 2>/dev/null)
echo "返回: $RESULT"

if echo "$RESULT" | grep -q "zhangsan"; then
    echo "✅ name 复制成功!"
else
    echo "❌ name 复制失败"
    echo "主节点日志:"
    tail -10 /tmp/kv_master_phase3.log
    echo ""
    echo "从节点日志:"
    tail -10 /tmp/kv_slave_phase3.log
    kill $MASTER_PID $SLAVE_PID 2>/dev/null
    exit 1
fi

echo "从从节点读取 'age':"
RESULT=$(./build/kv_client --host 127.0.0.1 --port 6381 <<< "GET age" 2>/dev/null)
echo "返回: $RESULT"

if echo "$RESULT" | grep -q "25"; then
    echo "✅ age 复制成功!"
else
    echo "❌ age 复制失败"
    kill $MASTER_PID $SLAVE_PID 2>/dev/null
    exit 1
fi

# 测试删除操作
echo "5. 测试删除操作复制..."
echo "在主节点删除 'age':"
./build/kv_client --host 127.0.0.1 --port 6380 <<< "DELETE age" 2>/dev/null

sleep 1

echo "在从节点检查 'age' 是否被删除:"
RESULT=$(./build/kv_client --host 127.0.0.1 --port 6381 <<< "GET age" 2>/dev/null)
if echo "$RESULT" | grep -q "ERROR"; then
    echo "✅ 删除操作复制成功!"
else
    echo "❌ 删除操作复制失败: $RESULT"
fi

# 清理
echo "6. 清理测试环境..."
kill $MASTER_PID $SLAVE_PID 2>/dev/null
sleep 1

echo ""
echo "=== 阶段三快速测试完成 ==="
echo "所有测试通过！"
echo "日志文件:"
echo "  /tmp/kv_master_phase3.log"
echo "  /tmp/kv_slave_phase3.log"
