#!/bin/bash

# ============================================
# 分片功能测试脚本
# 验证数据如何分布到不同分片
# ============================================

echo "🔀 分片功能测试"
echo "   验证数据分布到多个分片节点"
echo ""

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PHASE2_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"
PROJECT_DIR="$( cd "$PHASE2_DIR/../.." && pwd )"
BUILD_DIR="$PROJECT_DIR/build"
TEST_DATA_DIR="$PHASE2_DIR/data"

cd "$BUILD_DIR"

# 创建测试数据目录
mkdir -p "$TEST_DATA_DIR"

# 测试配置
NUM_TEST_KEYS=30
TEST_KEY_PREFIX="shard_key"

echo "   测试配置:"
echo "     测试键数量: $NUM_TEST_KEYS"
echo "     键前缀: $TEST_KEY_PREFIX"
echo "     测试数据目录: $TEST_DATA_DIR"
echo ""

# 1. 写入测试数据
echo "   1. 写入测试数据到集群..."
echo -n "   "

KEY_VALUE_FILE="$TEST_DATA_DIR/key_values.txt"
ROUTING_FILE="$TEST_DATA_DIR/routing_info.txt"

> "$KEY_VALUE_FILE"  # 清空文件
> "$ROUTING_FILE"    # 清空文件

for ((i=1; i<=NUM_TEST_KEYS; i++)); do
    key="${TEST_KEY_PREFIX}_${i}"
    value="value_$(printf "%03d" $i)_$(date +%s%N)"
    
    # 执行set操作
    output=$(./kv_client put "$key" "$value" 2>&1)
    
    # 记录键值对
    echo "$key=$value" >> "$KEY_VALUE_FILE"
    
    # 提取路由信息（如果客户端输出的话）
    if [[ "$output" == *"routed to"* ]]; then
        echo "$key -> $output" >> "$ROUTING_FILE"
    fi
    
    # 进度显示
    if (( i % 5 == 0 )); then
        echo -n "✓"
    else
        echo -n "."
    fi
done

echo ""
echo "   ✅ 测试数据写入完成"
echo "      键值对文件: $KEY_VALUE_FILE"
if [ -s "$ROUTING_FILE" ]; then
    echo "      路由信息文件: $ROUTING_FILE"
fi
echo ""

# 2. 验证数据可读取
echo "   2. 验证数据读取..."
echo -n "   "

VALIDATION_FILE="$TEST_DATA_DIR/validation_results.txt"
> "$VALIDATION_FILE"

CORRECT_COUNT=0
ERROR_COUNT=0

while IFS='=' read -r key expected_value; do
    # 跳过空行
    [ -z "$key" ] && continue
    
    # 执行GET操作
    actual_value=$(./kv_client get "$key" 2>/dev/null)
    
    # 验证结果
    if [ "$actual_value" = "$expected_value" ]; then
        echo "✅ $key: 正确" >> "$VALIDATION_FILE"
        ((CORRECT_COUNT++))
        echo -n "✓"
    else
        echo "❌ $key: 错误" >> "$VALIDATION_FILE"
        echo "   期望: $expected_value" >> "$VALIDATION_FILE"
        echo "   实际: $actual_value" >> "$VALIDATION_FILE"
        ((ERROR_COUNT++))
        echo -n "✗"
    fi
done < "$KEY_VALUE_FILE"

echo ""
echo ""
echo "   验证结果:"
echo "     正确: $CORRECT_COUNT"
echo "     错误: $ERROR_COUNT"
echo "     准确率: $((CORRECT_COUNT * 100 / NUM_TEST_KEYS))%"
echo ""

# 3. 分析分片分布
echo "   3. 分析分片分布情况..."

# 检查每个服务器的日志，统计处理了多少个key
echo "     各服务器处理统计:"
for port in 6381 6382 6383; do
    log_file="$PHASE2_DIR/logs/servers/server_$port.log"
    if [ -f "$log_file" ]; then
        key_count=$(grep -c "Processing.*$TEST_KEY_PREFIX" "$log_file" 2>/dev/null || echo "0")
        echo "       服务器 $port: $key_count 个键"
    fi
done

# 4. 生成分片分析报告
echo ""
echo "📊 分片测试报告"
echo "   ================="
echo "   总测试键数: $NUM_TEST_KEYS"
echo "   读取验证正确: $CORRECT_COUNT"
echo "   读取验证错误: $ERROR_COUNT"
echo ""

if [ $ERROR_COUNT -eq 0 ]; then
    echo "✅ 分片功能测试通过!"
    echo "   所有数据正确存储和读取"
    
    # 显示分布信息
    echo ""
    echo "📈 数据分布情况:"
    for port in 6381 6382 6383; do
        log_file="$PHASE2_DIR/logs/servers/server_$port.log"
        if [ -f "$log_file" ]; then
            key_count=$(grep -c "Processing.*$TEST_KEY_PREFIX" "$log_file" 2>/dev/null || echo "0")
            percentage=$((key_count * 100 / NUM_TEST_KEYS))
            echo "     服务器 $port: $key_count 键 ($percentage%)"
        fi
    done
    
    exit 0
else
    echo "❌ 分片测试发现 $ERROR_COUNT 个错误"
    echo "   详细错误信息: $VALIDATION_FILE"
    exit 1
fi
