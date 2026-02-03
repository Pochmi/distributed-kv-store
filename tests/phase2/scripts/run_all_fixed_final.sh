#!/bin/bash

echo "╔══════════════════════════════════════════╗"
echo "║   分布式KV存储系统 - 阶段二测试          ║"
echo "║   Distributed KV Store - Phase 2 Test    ║"
echo "╚══════════════════════════════════════════╝"
echo ""

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PHASE2_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"
PROJECT_DIR="$( cd "$PHASE2_DIR/../.." && pwd )"
BUILD_DIR="$PROJECT_DIR/build"

echo "📁 目录信息:"
echo "   项目目录: $PROJECT_DIR"
echo "   构建目录: $BUILD_DIR"
echo "   阶段二目录: $PHASE2_DIR"
echo ""

# 1. 清理旧进程
echo "🔍 清理旧进程..."
pkill -f "kv_server" 2>/dev/null
sleep 1

# 2. 检查编译
echo "🛠️  检查编译..."
if [ ! -f "$BUILD_DIR/kv_client" ] || [ ! -f "$BUILD_DIR/kv_server" ]; then
    echo "   重新编译..."
    cd "$BUILD_DIR"
    make -j4 > "$PHASE2_DIR/logs/compile_run_all.log" 2>&1
    if [ $? -ne 0 ]; then
        echo "❌ 编译失败"
        exit 1
    fi
    cd "$PROJECT_DIR"
fi

echo "✅ 编译检查通过"
echo ""

# 3. 启动3个分片服务器
echo "🚀 启动分布式测试集群..."
echo "   启动3个分片服务器节点"

SERVER_PIDS=()
PORTS=(6381 6382 6383)
NODE_IDS=("shard-1" "shard-2" "shard-3")

# 创建日志目录
mkdir -p "$PHASE2_DIR/logs/servers_final"

for i in {0..2}; do
    PORT=${PORTS[$i]}
    NODE_ID=${NODE_IDS[$i]}
    LOG_FILE="$PHASE2_DIR/logs/servers_final/server_$PORT.log"
    
    echo "   启动节点 $NODE_ID (127.0.0.1:$PORT)..."
    
    # 正确的启动方式：直接传递端口号和ID
    "$BUILD_DIR/kv_server" "$PORT" "$NODE_ID" > "$LOG_FILE" 2>&1 &
    
    SERVER_PIDS+=($!)
    sleep 2  # 给服务器更多时间启动
    
    # 检查服务器是否启动成功
    if ! ps -p ${SERVER_PIDS[$i]} > /dev/null 2>&1; then
        echo "❌ 节点 $NODE_ID 启动失败"
        echo "   查看日志: $LOG_FILE"
        echo "   日志内容:"
        tail -10 "$LOG_FILE"
        
        # 停止所有已启动的服务器
        for pid in "${SERVER_PIDS[@]}"; do
            kill $pid 2>/dev/null
        done
        exit 1
    fi
done

echo "✅ 集群启动完成 (PID: ${SERVER_PIDS[@]})"
echo "   服务器日志: $PHASE2_DIR/logs/servers_final/"
echo ""

# 4. 等待集群就绪
echo "⏳ 等待集群就绪..."
sleep 3

# 5. 运行基本功能测试
echo "🧪 运行基本功能测试..."
"$SCRIPT_DIR/test_basic.sh"
BASIC_RESULT=$?

if [ $BASIC_RESULT -eq 0 ]; then
    echo "✅ 基本功能测试通过"
else
    echo "❌ 基本功能测试失败"
fi
echo ""

# 6. 运行分片功能测试
echo "🔀 运行分片功能测试..."
"$SCRIPT_DIR/test_sharding.sh"
SHARDING_RESULT=$?

if [ $SHARDING_RESULT -eq 0 ]; then
    echo "✅ 分片功能测试通过"
else
    echo "❌ 分片功能测试失败"
fi
echo ""

# 7. 停止服务器集群
echo "🛑 停止测试集群..."
for PID in "${SERVER_PIDS[@]}"; do
    if kill -0 $PID 2>/dev/null; then
        kill $PID 2>/dev/null
        echo "   停止进程 $PID"
    fi
done

sleep 2

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
    
    # 生成成功报告
    REPORT_FILE="$PHASE2_DIR/final_test_report_$(date +%Y%m%d_%H%M%S).txt"
    echo "分布式KV存储 - 阶段二完整测试报告" > "$REPORT_FILE"
    echo "测试时间: $(date)" >> "$REPORT_FILE"
    echo "测试结果: 全部通过" >> "$REPORT_FILE"
    echo "详细日志: $PHASE2_DIR/logs/" >> "$REPORT_FILE"
    
    echo ""
    echo "📄 详细报告: $REPORT_FILE"
    exit 0
else
    echo "⚠️  阶段二测试部分失败"
    echo "   请查看详细日志: $PHASE2_DIR/logs/"
    exit 1
fi
