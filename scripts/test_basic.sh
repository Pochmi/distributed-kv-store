#!/bin/bash
# scripts/test_basic.sh

echo "=== 阶段一：基本功能测试 ==="
echo

# 编译
echo "1. 编译项目..."
mkdir -p build
cd build
cmake ..
make -j4

if [ $? -ne 0 ]; then
    echo "编译失败！"
    exit 1
fi

echo "编译成功！"
echo

# 启动服务器（后台运行）
echo "2. 启动服务器..."
./kv_server 6380 &
SERVER_PID=$!
sleep 2  # 等待服务器启动

echo "服务器已启动 (PID: $SERVER_PID)"
echo

# 运行基本测试
echo "3. 运行基本功能测试..."
echo "测试 SET 命令..."
./kv_client 127.0.0.1 6380 << EOF
SET name Alice
SET age 25
SET city Beijing
EOF

echo
echo "测试 GET 命令..."
./kv_client 127.0.0.1 6380 << EOF
GET name
GET age
GET city
GET notexist
EOF

echo
echo "测试 EXISTS 命令..."
./kv_client 127.0.0.1 6380 << EOF
EXISTS name
EXISTS notexist
EOF

echo
echo "测试 DEL 命令..."
./kv_client 127.0.0.1 6380 << EOF
DEL age
GET age
EOF

echo
echo "测试 PING 命令..."
./kv_client 127.0.0.1 6380 << EOF
PING
EOF

echo
echo "4. 清理..."
kill $SERVER_PID
wait $SERVER_PID 2>/dev/null

echo
echo "=== 测试完成 ==="
