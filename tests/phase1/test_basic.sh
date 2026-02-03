#!/bin/bash
# test_basic.sh - 修复路径问题的基础功能测试

echo "🧪 分布式KV存储系统 - 基础功能测试"
echo "========================================"
echo

# ==================== 固定项目路径 ====================
PROJECT_DIR="$HOME/桌面/distributed-kv-store"
cd "$PROJECT_DIR"

if [ $? -ne 0 ]; then
    echo "❌ 错误：无法进入项目目录 $PROJECT_DIR"
    exit 1
fi

echo "📁 项目目录: $(pwd)"
echo

# ==================== 环境检查 ====================
echo "🔍 环境检查..."
echo "----------------------------------------"

# 检查构建
if [ ! -d "build" ]; then
    echo "❌ 错误：找不到 build 目录"
    echo "请先运行: ./scripts/build.sh"
    exit 1
fi

cd build

if [ ! -f "kv_server" ]; then
    echo "❌ 错误：找不到 kv_server"
    exit 1
fi

echo "✅ 构建环境正常"
echo "✅ 可执行文件: $(ls kv_*)"
echo

# 检查网络工具
if ! command -v nc >/dev/null 2>&1 && ! command -v telnet >/dev/null 2>&1; then
    echo "⚠️  警告：没有找到 nc 或 telnet"
    echo "尝试安装 netcat..."
    sudo apt-get install -y netcat-openbsd 2>/dev/null || {
        echo "❌ 无法安装 netcat，请手动安装"
        exit 1
    }
fi

# ==================== 清理环境 ====================
echo "🧹 清理环境..."
echo "----------------------------------------"

# 停止旧进程
pkill -f "kv_server" 2>/dev/null && echo "停止旧服务器进程"
sleep 2

# 清理日志
rm -f test_basic.log test_output.txt server.log
echo "✅ 环境清理完成"
echo

# ==================== 启动服务器 ====================
echo "🚀 启动服务器..."
echo "----------------------------------------"

TEST_PORT=6391
LOG_FILE="test_basic.log"

echo "端口: $TEST_PORT"
echo "日志: $LOG_FILE"
echo

./kv_server $TEST_PORT > "$LOG_FILE" 2>&1 &
SERVER_PID=$!
echo "服务器PID: $SERVER_PID"

# 等待启动
echo -n "等待启动"
for i in {1..5}; do
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
echo "✅ 服务器运行中"
echo

# ==================== 定义测试函数 ====================
run_test() {
    local test_name="$1"
    local command="$2"
    local expected="$3"
    local description="$4"
    
    echo "测试 $test_name: $description"
    echo "  命令: $command"
    echo -n "  响应: "
    
    # 每个命令单独发送
    local response
    response=$(echo "$command" | timeout 3 nc -w 2 127.0.0.1 $TEST_PORT 2>/dev/null)
    
    echo "$response"
    
    # 验证结果
    if echo "$response" | grep -q "$expected"; then
        echo "  ✅ 通过"
        return 0
    else
        echo "  ❌ 失败 - 期望包含: $expected"
        return 1
    fi
    echo
}

# ==================== 执行测试用例 ====================
echo "🧪 执行测试用例..."
echo "========================================"
echo

TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# 测试组1: 基本CRUD操作
echo "📋 测试组1: 基本CRUD操作"
echo "----------------------------------------"

run_test "TC001" "SET user1 john" "OK" "设置键值对" && ((PASSED_TESTS++)) || ((FAILED_TESTS++))
((TOTAL_TESTS++))
sleep 0.3

run_test "TC002" "GET user1" "john" "获取存在的键" && ((PASSED_TESTS++)) || ((FAILED_TESTS++))
((TOTAL_TESTS++))
sleep 0.3

run_test "TC003" "EXISTS user1" "true" "检查存在的键" && ((PASSED_TESTS++)) || ((FAILED_TESTS++))
((TOTAL_TESTS++))
sleep 0.3

run_test "TC004" "DEL user1" "OK" "删除键" && ((PASSED_TESTS++)) || ((FAILED_TESTS++))
((TOTAL_TESTS++))
sleep 0.3

run_test "TC005" "GET user1" "ERROR" "获取不存在的键" && ((PASSED_TESTS++)) || ((FAILED_TESTS++))
((TOTAL_TESTS++))
sleep 0.3

echo

# 测试组2: 特殊值测试
echo "📋 测试组2: 特殊值测试"
echo "----------------------------------------"

run_test "TC006" "SET empty \"\"" "OK" "设置空值" && ((PASSED_TESTS++)) || ((FAILED_TESTS++))
((TOTAL_TESTS++))
sleep 0.3

run_test "TC007" "SET spaced \"hello world\"" "OK" "设置带空格的值" && ((PASSED_TESTS++)) || ((FAILED_TESTS++))
((TOTAL_TESTS++))
sleep 0.3

run_test "TC008" "GET spaced" "hello world" "获取带空格的值" && ((PASSED_TESTS++)) || ((FAILED_TESTS++))
((TOTAL_TESTS++))
sleep 0.3

echo

# 测试组3: 系统命令
echo "📋 测试组3: 系统命令"
echo "----------------------------------------"

run_test "TC009" "PING" "PONG" "PING命令" && ((PASSED_TESTS++)) || ((FAILED_TESTS++))
((TOTAL_TESTS++))
sleep 0.3

echo

# 测试组4: 错误处理
echo "📋 测试组4: 错误处理"
echo "----------------------------------------"

run_test "TC010" "SET" "ERROR" "SET缺少参数" && ((PASSED_TESTS++)) || ((FAILED_TESTS++))
((TOTAL_TESTS++))
sleep 0.3

run_test "TC011" "GET" "ERROR" "GET缺少参数" && ((PASSED_TESTS++)) || ((FAILED_TESTS++))
((TOTAL_TESTS++))
sleep 0.3

run_test "TC012" "UNKNOWN_CMD" "ERROR" "未知命令" && ((PASSED_TESTS++)) || ((FAILED_TESTS++))
((TOTAL_TESTS++))
sleep 0.3

# ==================== 简单并发测试 ====================
echo
echo "⚡ 简单并发测试"
echo "----------------------------------------"

echo "启动3个并发SET操作..."
for i in {1..3}; do
    (echo "SET concurrent$i value$i" | nc -w 1 127.0.0.1 $TEST_PORT >/dev/null 2>&1 && echo "  ✅ 线程$i 完成") &
done
wait
sleep 1

echo "验证并发结果..."
errors=0
for i in {1..3}; do
    result=$(echo "GET concurrent$i" | timeout 1 nc -w 1 127.0.0.1 $TEST_PORT 2>/dev/null)
    if echo "$result" | grep -q "value$i"; then
        echo "  ✅ concurrent$i 正确"
    else
        echo "  ❌ concurrent$i 错误: $result"
        ((errors++))
    fi
done

if [ $errors -eq 0 ]; then
    echo "✅ 并发测试通过"
    ((PASSED_TESTS++))
else
    echo "❌ 并发测试失败 ($errors 个错误)"
    ((FAILED_TESTS++))
fi
((TOTAL_TESTS++))

# ==================== 统计结果 ====================
echo
echo "========================================"
echo "📊 测试结果统计"
echo "========================================"
echo
echo "总测试用例: $TOTAL_TESTS"
echo "✅ 通过: $PASSED_TESTS"
echo "❌ 失败: $FAILED_TESTS"
echo
echo "通过率: $((PASSED_TESTS * 100 / TOTAL_TESTS))%"

if [ $FAILED_TESTS -eq 0 ]; then
    echo
    echo "🎉 所有测试通过！"
    echo "✅ 阶段一功能验证完成"
else
    echo
    echo "⚠️  有 $FAILED_TESTS 个测试失败"
    echo "请查看详细日志进行调试"
fi

# ==================== 清理 ====================
echo
echo "🧹 清理环境..."
echo "----------------------------------------"

echo "停止服务器 (PID: $SERVER_PID)"
kill $SERVER_PID 2>/dev/null
sleep 2

echo
echo "📋 测试日志摘要："
echo "----------------------------------------"
if [ -f "$LOG_FILE" ]; then
    echo "日志文件: $LOG_FILE"
    echo "文件大小: $(wc -l < "$LOG_FILE") 行"
    echo
    echo "关键日志（最后15行）:"
    tail -15 "$LOG_FILE"
else
    echo "❌ 日志文件不存在"
fi

# ==================== 保存测试报告 ====================
REPORT_FILE="test_basic_report_$(date +%Y%m%d_%H%M%S).txt"
cat > "$REPORT_FILE" << EOF
分布式KV存储系统 - 基础功能测试报告
======================================
测试时间: $(date)
测试端口: $TEST_PORT
测试用例数: $TOTAL_TESTS
通过用例: $PASSED_TESTS
失败用例: $FAILED_TESTS
通过率: $((PASSED_TESTS * 100 / TOTAL_TESTS))%

测试项目:
1. 基本CRUD操作 (TC001-TC005)
2. 特殊值测试 (TC006-TC008)
3. 系统命令测试 (TC009)
4. 错误处理测试 (TC010-TC012)
5. 并发测试

详细日志: $LOG_FILE
EOF

echo
echo "📄 测试报告已保存: $REPORT_FILE"
echo
echo "========================================"
echo "          基础功能测试完成"
echo "========================================"
