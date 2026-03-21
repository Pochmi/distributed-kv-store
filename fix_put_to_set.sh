#!/bin/bash

echo "=== 一键修复 PUT -> SET ==="
echo ""

# 1. 修改客户端核心文件
echo "1. 修改客户端 kv_client.cc..."
sed -i 's/"PUT " + key + " " + value/"SET " + key + " " + value/g' src/client/kv_client.cc
echo "   ✅ 已修改"

# 2. 修改服务器协议解析
echo "2. 修改服务器 main_server.cc..."
sed -i 's/request.substr(0, 3) == "PUT"/request.substr(0, 3) == "SET"/g' src/main_server.cc
sed -i 's/"Invalid PUT format"/"Invalid SET format"/g' src/main_server.cc
echo "   ✅ 已修改"

# 3. 修改测试脚本
echo "3. 修改测试脚本..."
if [ -f "tests/phase3/quick_test.sh" ]; then
    sed -i 's/PUT name/SET name/g' tests/phase3/quick_test.sh
    sed -i 's/PUT age/SET age/g' tests/phase3/quick_test.sh
    echo "   ✅ 修改 phase3/quick_test.sh"
fi

if [ -f "tests/phase2/scripts/quick_test.sh" ]; then
    sed -i 's/PUT操作/SET操作/g' tests/phase2/scripts/quick_test.sh
    sed -i 's/PUT key/SET key/g' tests/phase2/scripts/quick_test.sh
    echo "   ✅ 修改 phase2/scripts/quick_test.sh"
fi

if [ -f "tests/unit/test_kv_store.cc" ]; then
    sed -i 's/测试PUT/测试SET/g' tests/unit/test_kv_store.cc
    echo "   ✅ 修改 tests/unit/test_kv_store.cc"
fi

# 4. 重新编译
echo ""
echo "4. 重新编译..."
cd build
make -j4 2>&1 | tail -10
cd ..

echo ""
echo "✅ 修复完成！"
echo ""
echo "现在重新启动主从节点："
echo "  ./build/bin/kv_server --port 6381 --role master --replication"
echo "  ./build/bin/kv_server --port 6382 --role slave --master 127.0.0.1:6381 --replication"
echo ""
echo "然后测试："
echo "  ./build/bin/kv_client set name '张三'"
