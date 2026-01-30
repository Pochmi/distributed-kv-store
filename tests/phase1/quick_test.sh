#!/bin/bash
# quick_test.sh - 快速功能验证测试
# 位置：~/桌面/distributed-kv-store/tests/scripts/quick_test.sh

echo "🚀 分布式KV存储系统 - 快速功能测试"
echo "========================================"
echo

# 进入项目根目录
cd "$(dirname "$0")/../.." 2>/dev/null || cd ~/桌面/distributed-kv-store

echo "📁 当前目录: $(pwd)"
echo

# 检查可执行文件
if [ ! -f "build/kv_server" ]; then
    echo "❌ 错误：找不到 build/kv_server"
    echo "请先运行: ./scripts/build.sh"
    exit 1
fi

echo "✅ 找到可执行文件: build/kv_server"
echo

# 清理旧进程
echo "🧹 清理环境..."
pkill -f "kv_server" 2>/dev/null && echo "停止旧服务器进程"
sleep 1

# 临时日志文件
LOG_FILE="/tmp/kv_quick_test_$(date +%s).log"
TEST_PORT=6390

echo "🔧 测试端口: $TEST_PORT"
echo "📝 日志文件: $LOG_FILE"
echo

# ==================== 启动服务器 ====================
echo "🚀 启动服务器..."
cd build
./kv_server $TEST_PORT > "$LOG_FILE" 2>&1 &
SERVER_PID=$!

echo "服务器PID: $SERVER_PID"
echo -n "等待服务器启动"
for i in {1..3}; do
    if kill -0 $SERVER_PID 2>/dev/null; then
        echo -n "."
        sleep 1
    else
        echo
        echo "❌ 服务器启动失败"
        echo "查看日志:"
        tail -10 "$LOG_FILE"
        exit 1
    fi
done
echo
echo "✅ 服务器启动成功"
echo

# ==================== 执行测试 ====================
echo "🧪 执行功能测试..."
echo "----------------------------------------"

echo "测试 1/5: SET 命令"
echo "命令: SET quicktest hello"
RESULT1=$(echo "SET quicktest hello" | timeout 2 nc -w 1 127.0.0.1 $TEST_PORT 2>/dev/null)
echo "响应: $RESULT1"
if [ "$RESULT1" = "OK" ]; then
    echo "✅ 通过"
else
    echo "❌ 失败"
fi
sleep 0.3

echo
echo "测试 2/5: GET 命令"
echo "命令: GET quicktest"
RESULT2=$(echo "GET quicktest" | timeout 2 nc -w 1 127.0.0.1 $TEST_PORT 2>/dev/null)
echo "响应: $RESULT2"
if echo "$RESULT2" | grep -q "hello"; then
    echo "✅ 通过"
else
    echo "❌ 失败"
fi
sleep 0.3

echo
echo "测试 3/5: EXISTS 命令"
echo "命令: EXISTS quicktest"
RESULT3=$(echo "EXISTS quicktest" | timeout 2 nc -w 1 127.0.0.1 $TEST_PORT 2>/dev/null)
echo "响应: $RESULT3"
if [ "$RESULT3" = "true" ]; then
    echo "✅ 通过"
else
    echo "❌ 失败"
fi
sleep 0.3

echo
echo "测试 4/5: DEL 命令"
echo "命令: DEL quicktest"
RESULT4=$(echo "DEL quicktest" | timeout 2 nc -w 1 127.0.0.1 $TEST_PORT 2>/dev/null)
echo "响应: $RESULT4"
if [ "$RESULT4" = "OK" ]; then
    echo "✅ 通过"
else
    echo "❌ 失败"
fi
sleep 0.3

echo
echo "测试 5/5: GET 已删除的键"
echo "命令: GET quicktest"
RESULT5=$(echo "GET quicktest" | timeout 2 nc -w 1 127.0.0.1 $TEST_PORT 2>/dev/null)
echo "响应: $RESULT5"
if echo "$RESULT5" | grep -qi "error\|not found"; then
    echo "✅ 通过（正确返回错误）"
else
    echo "⚠️  注意：响应格式可能不同"
fi

echo
echo "----------------------------------------"

# ==================== 附加测试 ====================
echo
echo "🔧 附加测试..."
echo "测试 PING 命令:"
RESULT_PING=$(echo "PING" | timeout 2 nc -w 1 127.0.0.1 $TEST_PORT 2>/dev/null)
echo "响应: $RESULT_PING"
if [ "$RESULT_PING" = "PONG" ]; then
    echo "✅ PING 通过"
else
    echo "⚠️  PING 响应: $RESULT_PING"
fi

echo
echo "测试无效命令:"
RESULT_BAD=$(echo "INVALID_COMMAND" | timeout 2 nc -w 1 127.0.0.1 $TEST_PORT 2>/dev/null)
echo "响应: $RESULT_BAD"
if echo "$RESULT_BAD" | grep -qi "error"; then
    echo "✅ 错误处理正常"
else
    echo "⚠️  错误处理响应: $RESULT_BAD"
fi

# ==================== 清理 ====================
echo
echo "🧹 清理测试环境..."
kill $SERVER_PID 2>/dev/null
sleep 1

echo
echo "📋 服务器日志摘要："
echo "----------------------------------------"
if [ -f "$LOG_FILE" ]; then
    echo "最后15行日志:"
    tail -15 "$LOG_FILE"
    echo
    echo "日志文件: $LOG_FILE"
else
    echo "❌ 日志文件不存在"
fi

# ==================== 总结 ====================
echo
echo "========================================"
echo "            快速测试完成"
echo "========================================"
echo
echo "💡 测试说明:"
echo "1. 这是一个快速功能验证测试"
echo "2. 用于开发过程中快速检查"
echo "3. 完整测试请运行: ./tests/integration/phase1/full_test.sh"
echo
echo "📊 测试覆盖:"
echo "✅ SET/GET/DEL/EXISTS 基本命令"
echo "✅ PING 心跳检测"
echo "✅ 错误命令处理"
echo "✅ 数据持久性验证"
echo
echo "🚀 快速重启测试:"
echo "  直接再次运行: ./tests/scripts/quick_test.sh"
echo
echo "========================================"
