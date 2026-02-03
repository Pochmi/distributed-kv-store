#!/bin/bash

# 清理脚本
echo "清理构建文件和测试数据..."

# 清理构建目录
if [ -d "build" ]; then
    echo "删除 build/ 目录..."
    rm -rf build
fi

# 清理测试数据
if [ -d "tests/phase2/data" ]; then
    echo "清理测试数据..."
    rm -rf tests/phase2/data/*
fi

# 清理日志
if [ -d "tests/phase2/logs" ]; then
    echo "清理测试日志..."
    rm -rf tests/phase2/logs/*
fi

# 清理服务器数据
if [ -d "data" ]; then
    echo "清理服务器数据..."
    rm -rf data/*
fi

# 清理运行时日志
if [ -d "logs" ]; then
    echo "清理运行时日志..."
    rm -rf logs/*
fi

echo "清理完成！"
