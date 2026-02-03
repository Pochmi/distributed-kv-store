#!/bin/bash

# ============================================
# 基本功能测试脚本
# 测试PUT、GET、DELETE操作
# ============================================

echo "🧪 基本功能测试"
echo "   验证KV存储的基本操作"
echo ""

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PHASE2_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"
PROJECT_DIR="$( cd "$PHASE2_DIR/../.." && pwd )"
BUILD_DIR="$PROJECT_DIR/build"

cd "$BUILD_DIR"

# 测试结果统计
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# 测试函数
run_test() {
    local test_name="$1"
    local command="$2"
    local expected="$3"
    
    ((TOTAL_TESTS++))
    
    echo "   🔹 测试: $test_name"
    echo "      命令: $command"
    
    # 执行命令
    output=$(eval "$command" 2>&1)
    local result=$?
    
    # 检查结果
    if [ $result -eq 0 ]; then
        if [ -n "$expected" ] && [[ "$output" != *"$expected"* ]]; then
            echo "      结果: ❌ 失败"
            echo "      输出: $output"
            echo "      期望包含: $expected"
            ((FAILED_TESTS++))
        else
            echo "      结果: ✅ 通过"
            ((PASSED_TESTS++))
        fi
    else
        echo "      结果: ❌ 失败 (退出码: $result)"
        echo "      输出: $output"
        ((FAILED_TESTS++))
    fi
    
    echo ""
}

# 1. 测试PUT操作
echo "   1. SET操作测试"
echo "   ----------------"

run_test "SET字符串" "./kv_client set test_string 'Hello World'" "successful"
run_test "SET数字" "./kv_client set test_number 12345" "successful"
run_test "SET中文" "./kv_client set test_chinese '分布式系统'" "successful"
run_test "SET特殊字符" "./kv_client set test_special 'key=value&data=test'" "successful"

# 2. 测试GET操作
echo "   2. GET操作测试"
echo "   ----------------"

run_test "GET存在的键" "./kv_client get test_string" "Hello World"
run_test "GET不存在的键" "./kv_client get non_existent_key" "not found"

# 3. 测试DELETE操作
echo "   3. DELETE操作测试"
echo "   -------------------"

run_test "DELETE存在的键" "./kv_client del test_number" "successful"
run_test "验证DELETE" "./kv_client get test_number" "not found"
run_test "DELETE不存在的键" "./kv_client del non_existent_key" "failed"

# 4. 测试综合场景
echo "   4. 综合场景测试"
echo "   ----------------"

run_test "SET-GET-DELETE流程" \
    "./kv_client set flow_test 'test_value' && \
     ./kv_client get flow_test && \
     ./kv_client del flow_test && \
     ./kv_client get flow_test | grep -q 'not found'" \
    ""

# 测试报告
echo "📋 基本功能测试报告"
echo "   ================="
echo "   总测试数: $TOTAL_TESTS"
echo "   通过数: $PASSED_TESTS"
echo "   失败数: $FAILED_TESTS"
echo "   通过率: $((PASSED_TESTS * 100 / TOTAL_TESTS))%"
echo ""

if [ $FAILED_TESTS -eq 0 ]; then
    echo "✅ 所有基本功能测试通过!"
    exit 0
else
    echo "❌ 有 $FAILED_TESTS 个测试失败"
    exit 1
fi
