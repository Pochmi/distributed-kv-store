#!/bin/bash
# 启动一个主节点和两个从节点的复制集群

BASE_DIR=$(dirname "$0")/../..
BUILD_DIR="$BASE_DIR/build"
CONFIG_DIR="$BASE_DIR/configs"
LOG_DIR="$BASE_DIR/logs"

# 创建日志目录
mkdir -p "$LOG_DIR"

echo "Starting replication cluster (1 master + 2 slaves)..."

# 启动主节点 (端口 6380)
echo "Starting master node on port 6380..."
"$BUILD_DIR/kv_server" --config "$CONFIG_DIR/master.conf" > "$LOG_DIR/master.log" 2>&1 &
MASTER_PID=$!
echo "Master started with PID: $MASTER_PID"

sleep 2

# 启动从节点1 (端口 6381)
echo "Starting slave node 1 on port 6381..."
"$BUILD_DIR/kv_server" --config "$CONFIG_DIR/slave1.conf" > "$LOG_DIR/slave1.log" 2>&1 &
SLAVE1_PID=$!
echo "Slave 1 started with PID: $SLAVE1_PID"

# 启动从节点2 (端口 6382)
echo "Starting slave node 2 on port 6382..."
"$BUILD_DIR/kv_server" --config "$CONFIG_DIR/slave2.conf" > "$LOG_DIR/slave2.log" 2>&1 &
SLAVE2_PID=$!
echo "Slave 2 started with PID: $SLAVE2_PID"

# 保存PID文件
echo $MASTER_PID > "$LOG_DIR/master.pid"
echo $SLAVE1_PID > "$LOG_DIR/slave1.pid"
echo $SLAVE2_PID > "$LOG_DIR/slave2.pid"

echo "Cluster started!"
echo "Master: 127.0.0.1:6380"
echo "Slave1: 127.0.0.1:6381"
echo "Slave2: 127.0.0.1:6382"
