#!/bin/bash

# ============================================
# 分布式KV存储 - 阶段二测试主脚本
# 测试分布式分片功能
# ============================================

echo "╔══════════════════════════════════════════╗"
echo "║   分布式KV存储系统 - 阶段二测试          ║"
echo "║   Distributed KV Store - Phase 2 Test    ║"
echo "╚══════════════════════════════════════════╝"
echo ""

# 获取脚本所在目录
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PHASE2_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"
TESTS_DIR="$( cd "$PHASE2_DIR/.." && pwd )"
PROJECT_DIR="$( cd "$TESTS_DIR/.." && pwd )"
BUILD_DIR="$PROJECT_DIR/build"

echo "📁 目录信息:"
echo "   项目目录: $PROJECT_DIR"
echo "   构建目录: $BUILD_DIR"
echo "   测试目录: $TESTS_DIR"
echo "   阶段二目录: $PHASE2_DIR"
echo ""

# 1. 检查必要目录
echo "🔍 检查目录结构..."
if [ ! -d "$PROJECT_DIR/src" ]; then
    echo "❌ 错误: 找不到 src/ 目录"
    exit 1
fi

if [ ! -f "$PROJECT_DIR/CMakeLists.txt" ]; then
    echo "❌ 错误: 找不到 CMakeLists.txt"
    exit 1
fi

echo "✅ 目录结构检查通过"
echo ""

# 2. 清理和构建
echo "🛠️  步骤1: 清理和构建项目..."
cd "$PROJECT_DIR"

# 清理旧构建
if [ -d "$BUILD_DIR" ]; then
    echo "   清理旧构建文件..."
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# 配置CMake
echo "   配置CMake..."
cmake .. > "$PHASE2_DIR/logs/cmake.log" 2>&1
if [ $? -ne 0 ]; then
    echo "❌ CMake配置失败"
    echo "   查看日志: $PHASE2_DIR/logs/cmake.log"
    exit 1
fi

# 编译
echo "   编译项目..."
make -j4 > "$PHASE2_DIR/logs/make.log" 2>&1
if [ $? -ne 0 ]; then
    echo "❌ 编译失败"
    echo "   查看日志: $PHASE2_DIR/logs/make.log"
    exit 1
fi

echo "✅ 构建成功!"
echo "   可执行文件:"
ls -la kv_server kv_client 2>/dev/null || echo "   警告: 找不到可执行文件"
echo ""

# 3. 启动测试服务器集群
echo "🚀 步骤2: 启动分布式测试集群..."
echo "   启动3个分片服务器节点"

# 停止可能已经在运行的服务器
pkill -f "kv_server" 2>/dev/null && sleep 1

# 创建服务器日志目录
mkdir -p "$PHASE2_DIR/logs/servers"

# 启动服务器节点
SERVER_PIDS=()
PORTS=(6381 6382 6383)
NODE_IDS=("shard-1" "shard-2" "shard-3")

for i in {0..2}; do
    PORT=${PORTS[$i]}
    NODE_ID=${NODE_IDS[$i]}
    LOG_FILE="$PHASE2_DIR/logs/servers/server_$PORT.log"
    
    echo "   启动节点 $NODE_ID (127.0.0.1:$PORT)..."
    "$BUILD_DIR/kv_server" --port "$PORT" --id "$NODE_ID" \
        --data-dir "$PROJECT_DIR/data/$NODE_ID" \
        > "$LOG_FILE" 2>&1 &
    
    SERVER_PIDS+=($!)
    sleep 1
    
    # 检查服务器是否启动成功
    if ! kill -0 ${SERVER_PIDS[$i]} 2>/dev/null; then
        echo "❌ 节点 $NODE_ID 启动失败"
        echo "   查看日志: $LOG_FILE"
        exit 1
    fi
done

echo "✅ 集群启动完成 (PID: ${SERVER_PIDS[@]})"
echo "   服务器日志: $PHASE2_DIR/logs/servers/"
echo ""

# 4. 等待集群就绪
echo "⏳ 等待集群就绪..."
sleep 3

# 5. 运行基本功能测试
echo "🧪 步骤3: 运行基本功能测试..."
"$SCRIPT_DIR/test_basic.sh"
BASIC_RESULT=$?

if [ $BASIC_RESULT -eq 0 ]; then
    echo "✅ 基本功能测试通过"
else
    echo "❌ 基本功能测试失败"
fi
echo ""

# 6. 运行分片功能测试
echo "🔀 步骤4: 运行分片功能测试..."
"$SCRIPT_DIR/test_sharding.sh"
SHARDING_RESULT=$?

if [ $SHARDING_RESULT -eq 0 ]; then
    echo "✅ 分片功能测试通过"
else
    echo "❌ 分片功能测试失败"
fi
echo ""

# 7. 停止服务器集群
echo "🛑 步骤5: 停止测试集群..."
for PID in "${SERVER_PIDS[@]}"; do
    if kill -0 $PID 2>/dev/null; then
        kill $PID 2>/dev/null
        echo "   停止进程 $PID"
    fi
done

sleep 1

# 确保所有进程都被终止
pkill -9 -f "kv_server" 2>/dev/null || true

echo "✅ 集群已停止"
echo ""

# 8. 生成测试报告
echo "📊 测试报告"
echo "   =========================="

TOTAL_TESTS=2
PASSED_TESTS=0
[ $BASIC_RESULT -eq 0 ] && ((PASSED_TESTS++))
[ $SHARDING_RESULT -eq 0 ] && ((PASSED_TESTS++))

echo "   总测试项: $TOTAL_TESTS"
echo "   通过项: $PASSED_TESTS"
echo "   失败项: $((TOTAL_TESTS - PASSED_TESTS))"
echo ""

if [ $PASSED_TESTS -eq $TOTAL_TESTS ]; then
    echo "🎉 阶段二测试全部通过!"
    echo "   分布式分片功能实现成功"
    exit 0
else
    echo "⚠️  阶段二测试部分失败"
    echo "   请查看详细日志: $PHASE2_DIR/logs/"
    exit 1
fi
