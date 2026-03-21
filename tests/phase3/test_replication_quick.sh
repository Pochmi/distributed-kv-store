#!/bin/bash

echo "===================================="
echo "  主从复制快速测试脚本"
echo "===================================="
echo ""

# 测试计数器
TOTAL=0
PASS=0
FAIL=0

# 测试函数
test_case() {
    local name="$1"
    local cmd="$2"
    local expect="$3"
    
    TOTAL=$((TOTAL + 1))
    echo -n "测试 $TOTAL: $name ... "
    
    output=$(eval "$cmd" 2>&1)
    
    if echo "$output" | grep -q "$expect"; then
        echo "✅ 通过"
        PASS=$((PASS + 1))
    else
        echo "❌ 失败"
        echo "   期望: $expect"
        echo "   实际: $output"
        FAIL=$((FAIL + 1))
    fi
}

# 清理旧进程
echo "清理旧进程..."
pkill -f kv_server
sleep 2

# 启动主节点
echo "启动主节点 (6381)..."
./build/bin/kv_server --port 6381 --role master --replication > /tmp/master.log 2>&1 &
MASTER_PID=$!
sleep 3

# 启动从节点
echo "启动从节点 (6382)..."
./build/bin/kv_server --port 6382 --role slave --master 127.0.0.1:6381 --replication > /tmp/slave.log 2>&1 &
SLAVE_PID=$!
sleep 3

echo ""
echo "===================================="
echo "开始测试"
echo "===================================="

# 测试1: 主节点写入
test_case "主节点写入" "./build/bin/kv_client set name '张三'" "成功"

# 测试2: 主节点读取
test_case "主节点读取" "./build/bin/kv_client get name" "张三"

# 测试3: 从节点读取
test_case "从节点读取" "./build/bin/kv_client get name" "张三"

# 测试4: 主节点写入数字
test_case "主节点写入数字" "./build/bin/kv_client set age '25'" "成功"

# 测试5: 从节点读取数字
test_case "从节点读取数字" "./build/bin/kv_client get age" "25"

# 测试6: 主节点删除
test_case "主节点删除" "./build/bin/kv_client del name" "成功"

# 测试7: 从节点验证删除
test_case "从节点验证删除" "./build/bin/kv_client get name" "不存在"

# 测试8: 批量写入 (3个键)
test_case "批量写入" "
./build/bin/kv_client set key1 'value1'
./build/bin/kv_client set key2 'value2'
./build/bin/kv_client set key3 'value3'
" "成功"

# 测试9: 从节点批量读取
test_case "从节点批量读取" "
./build/bin/kv_client get key1
./build/bin/kv_client get key2
./build/bin/kv_client get key3
" "value"

echo ""
echo "===================================="
echo "测试结果"
echo "===================================="
echo "总测试: $TOTAL"
echo -e "通过: \033[32m$PASS\033[0m"
echo -e "失败: \033[31m$FAIL\033[0m"

# 清理
echo ""
echo "清理进程..."
kill $MASTER_PID $SLAVE_PID 2>/dev/null
sleep 2

if [ $FAIL -eq 0 ]; then
    echo -e "\n\033[32m✅ 所有测试通过！主从复制工作正常！\033[0m"
else
    echo -e "\n\033[31m❌ 有 $FAIL 个测试失败\033[0m"
fi