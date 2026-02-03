#!/bin/bash

echo "=== 阶段二文件设置脚本 ==="
echo "创建所有阶段二需要的源文件"
echo ""

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_DIR="$( cd "$SCRIPT_DIR/../.." && pwd )"
cd "$PROJECT_DIR"

echo "项目目录: $PROJECT_DIR"
echo ""

# 1. 备份原始文件
echo "1. 备份原始客户端文件..."
backup_dir="$SCRIPT_DIR/backup_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$backup_dir"

if [ -d "src/client" ]; then
    cp -r src/client/* "$backup_dir/" 2>/dev/null
    echo "  备份到: $backup_dir"
else
    mkdir -p src/client
    echo "  创建 src/client 目录"
fi

# 2. 创建日志目录
mkdir -p "$SCRIPT_DIR/logs"

# 3. 创建所有必要的源文件
echo "2. 创建阶段二源文件..."
echo "  这将创建8个源文件"
echo ""

# 创建公共工具文件
echo "创建 src/common/utils.h..."
cat > src/common/utils.h << 'EOF'
#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>

namespace utils {
    std::vector<std::string> Split(const std::string& str, char delimiter);
    std::string Trim(const std::string& str);
}

#endif
EOF

echo "创建 src/common/utils.cc..."
cat > src/common/utils.cc << 'EOF'
#include "utils.h"
#include <sstream>
#include <algorithm>
#include <cctype>

namespace utils {

std::vector<std::string> Split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);
    
    while (std::getline(tokenStream, token, delimiter)) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    
    return tokens;
}

std::string Trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

}
EOF

# 创建客户端连接文件
echo "创建 src/client/connection.h..."
cat > src/client/connection.h << 'EOF'
#ifndef CLIENT_CONNECTION_H
#define CLIENT_CONNECTION_H

#include <string>

class Connection {
public:
    Connection();
    ~Connection();
    
    bool connect(const std::string& host, int port);
    bool send(const std::string& data);
    bool receive(std::string& data);
    void close();
    
    bool isConnected() const { return sockfd_ >= 0; }
    
private:
    int sockfd_{-1};
};
#endif
EOF

echo "创建 src/client/connection.cc..."
cat > src/client/connection.cc << 'EOF'
#include "connection.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <iostream>

Connection::Connection() : sockfd_(-1) {}
Connection::~Connection() { close(); }

bool Connection::connect(const std::string& host, int port) {
    if (sockfd_ >= 0) close();
    
    sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_ < 0) {
        std::cerr << "Failed to create socket: " << strerror(errno) << std::endl;
        return false;
    }
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
        std::cerr << "Invalid address: " << host << std::endl;
        close();
        return false;
    }
    
    if (::connect(sockfd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Failed to connect to " << host << ":" << port 
                  << " - " << strerror(errno) << std::endl;
        close();
        return false;
    }
    
    return true;
}

bool Connection::send(const std::string& data) {
    if (sockfd_ < 0) {
        std::cerr << "Socket not connected" << std::endl;
        return false;
    }
    
    ssize_t bytes_sent = ::send(sockfd_, data.c_str(), data.length(), 0);
    if (bytes_sent < 0) {
        std::cerr << "Failed to send data: " << strerror(errno) << std::endl;
        return false;
    }
    
    return bytes_sent == static_cast<ssize_t>(data.length());
}

bool Connection::receive(std::string& data) {
    if (sockfd_ < 0) {
        std::cerr << "Socket not connected" << std::endl;
        return false;
    }
    
    char buffer[4096];
    ssize_t bytes_recv = ::recv(sockfd_, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_recv < 0) {
        std::cerr << "Failed to receive data: " << strerror(errno) << std::endl;
        return false;
    } else if (bytes_recv == 0) {
        std::cout << "Connection closed by server" << std::endl;
        return false;
    }
    
    buffer[bytes_recv] = '\0';
    data = std::string(buffer, bytes_recv);
    return true;
}

void Connection::close() {
    if (sockfd_ >= 0) {
        ::close(sockfd_);
        sockfd_ = -1;
    }
}
EOF

# 创建集群配置
echo "创建 src/client/cluster_config.h..."
cat > src/client/cluster_config.h << 'EOF'
#ifndef CLIENT_CLUSTER_CONFIG_H
#define CLIENT_CLUSTER_CONFIG_H

#include <string>
#include <vector>

struct NodeInfo {
    std::string id;
    std::string host;
    int port;
    std::string role;
    bool is_available{true};
    
    std::string address() const {
        return host + ":" + std::to_string(port);
    }
};

class ClusterConfig {
public:
    static ClusterConfig& getInstance();
    void initDefaultConfig();
    NodeInfo getNodeByKey(const std::string& key);
    std::vector<NodeInfo> getAllNodes() const { return nodes_; }
    
private:
    ClusterConfig() = default;
    std::vector<NodeInfo> nodes_;
};
#endif
EOF

echo "创建 src/client/cluster_config.cc..."
cat > src/client/cluster_config.cc << 'EOF'
#include "cluster_config.h"
#include <functional>
#include <iostream>

ClusterConfig& ClusterConfig::getInstance() {
    static ClusterConfig instance;
    return instance;
}

void ClusterConfig::initDefaultConfig() {
    nodes_ = {
        {"shard-1", "127.0.0.1", 6381, "master", true},
        {"shard-2", "127.0.0.1", 6382, "master", true},
        {"shard-3", "127.0.0.1", 6383, "master", true}
    };
    
    std::cout << "[Cluster] Initialized with " << nodes_.size() << " nodes" << std::endl;
}

NodeInfo ClusterConfig::getNodeByKey(const std::string& key) {
    if (nodes_.empty()) initDefaultConfig();
    
    std::hash<std::string> hash_fn;
    size_t hash_value = hash_fn(key);
    size_t node_index = hash_value % nodes_.size();
    
    return nodes_[node_index];
}
EOF

# 创建路由文件
echo "创建 src/client/router.h..."
cat > src/client/router.h << 'EOF'
#ifndef CLIENT_ROUTER_H
#define CLIENT_ROUTER_H

#include "cluster_config.h"
#include <string>

class Router {
public:
    Router();
    NodeInfo route(const std::string& key);
    
private:
    ClusterConfig& config_;
    uint32_t hash(const std::string& key);
};
#endif
EOF

echo "创建 src/client/router.cc..."
cat > src/client/router.cc << 'EOF'
#include "router.h"
#include <iostream>
#include <functional>

Router::Router() : config_(ClusterConfig::getInstance()) {
    config_.initDefaultConfig();
}

uint32_t Router::hash(const std::string& key) {
    return std::hash<std::string>{}(key);
}

NodeInfo Router::route(const std::string& key) {
    uint32_t hash_value = hash(key);
    auto all_nodes = config_.getAllNodes();
    
    if (all_nodes.empty()) {
        throw std::runtime_error("No nodes available in cluster");
    }
    
    size_t node_index = hash_value % all_nodes.size();
    NodeInfo target_node = all_nodes[node_index];
    
    std::cout << "[Router] Key '" << key << "' -> Node " 
              << target_node.id << " (" << target_node.address() << ")" << std::endl;
    
    return target_node;
}
EOF

# 创建客户端主类
echo "创建 src/client/kv_client.h..."
cat > src/client/kv_client.h << 'EOF'
#ifndef CLIENT_KV_CLIENT_H
#define CLIENT_KV_CLIENT_H

#include <string>

class KVClient {
public:
    KVClient();
    bool put(const std::string& key, const std::string& value);
    std::string get(const std::string& key);
    bool del(const std::string& key);
    
private:
    bool sendToNode(const std::string& host, int port, 
                   const std::string& request, 
                   std::string& response);
    
    std::string serializeRequest(const std::string& cmd, 
                                const std::string& key, 
                                const std::string& value = "");
};
#endif
EOF

echo "创建 src/client/kv_client.cc..."
cat > src/client/kv_client.cc << 'EOF'
#include "kv_client.h"
#include "connection.h"
#include "router.h"
#include <iostream>

KVClient::KVClient() {
    std::cout << "[KVClient] Initialized" << std::endl;
}

bool KVClient::put(const std::string& key, const std::string& value) {
    std::cout << "[KVClient] PUT " << key << " = " << value << std::endl;
    
    Router router;
    NodeInfo node;
    try {
        node = router.route(key);
    } catch (const std::exception& e) {
        std::cerr << "[KVClient] Routing failed: " << e.what() << std::endl;
        return false;
    }
    
    std::string request = serializeRequest("SET", key, value);
    std::string response;
    
    if (!sendToNode(node.host, node.port, request, response)) {
        return false;
    }
    
    std::cout << "[KVClient] Server response: " << response;
    return response.find("OK") != std::string::npos;
}

std::string KVClient::get(const std::string& key) {
    std::cout << "[KVClient] GET " << key << std::endl;
    
    Router router;
    NodeInfo node;
    try {
        node = router.route(key);
    } catch (const std::exception& e) {
        std::cerr << "[KVClient] Routing failed: " << e.what() << std::endl;
        return "";
    }
    
    std::string request = serializeRequest("GET", key);
    std::string response;
    
    if (!sendToNode(node.host, node.port, request, response)) {
        return "";
    }
    
    std::cout << "[KVClient] Server response: " << response;
    return response;
}

bool KVClient::del(const std::string& key) {
    std::cout << "[KVClient] DELETE " << key << std::endl;
    
    Router router;
    NodeInfo node;
    try {
        node = router.route(key);
    } catch (const std::exception& e) {
        std::cerr << "[KVClient] Routing failed: " << e.what() << std::endl;
        return false;
    }
    
    std::string request = serializeRequest("DEL", key);
    std::string response;
    
    if (!sendToNode(node.host, node.port, request, response)) {
        return false;
    }
    
    std::cout << "[KVClient] Server response: " << response;
    return response.find("OK") != std::string::npos;
}

bool KVClient::sendToNode(const std::string& host, int port, 
                         const std::string& request, 
                         std::string& response) {
    Connection conn;
    
    if (!conn.connect(host, port)) {
        std::cerr << "[KVClient] Failed to connect to " << host << ":" << port << std::endl;
        return false;
    }
    
    if (!conn.send(request)) {
        std::cerr << "[KVClient] Failed to send request" << std::endl;
        return false;
    }
    
    if (!conn.receive(response)) {
        std::cerr << "[KVClient] Failed to receive response" << std::endl;
        return false;
    }
    
    return true;
}

std::string KVClient::serializeRequest(const std::string& cmd, 
                                      const std::string& key, 
                                      const std::string& value) {
    if (cmd == "SET") {
        return "SET " + key + " " + value + "\n";
    } else if (cmd == "GET") {
        return "GET " + key + "\n";
    } else if (cmd == "DEL") {
        return "DEL " + key + "\n";
    }
    
    return "";
}
EOF

# 更新CMakeLists.txt
echo "3. 更新 CMakeLists.txt..."
if grep -q "src/common/utils.cc" CMakeLists.txt; then
    echo "  CMakeLists.txt 已包含 utils.cc"
else
    echo "  在 CMakeLists.txt 中添加 utils.cc"
    # 创建临时文件
    cp CMakeLists.txt CMakeLists.txt.bak
    # 简化：直接创建新的CMakeLists.txt
    cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.10)
project(kv_store)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pthread")

include_directories(src)

add_executable(kv_server
    src/main_server.cc
    src/common/logger.cc
    src/common/protocol.cc
    src/common/utils.cc
    src/core/memory_store.cc
    src/network/simple_server.cc
)

add_executable(kv_client
    src/main_client.cc
    src/common/logger.cc
    src/common/protocol.cc
    src/common/utils.cc
    src/client/kv_client.cc
    src/client/router.cc
    src/client/cluster_config.cc
    src/client/connection.cc
)

target_link_libraries(kv_server pthread)
target_link_libraries(kv_client pthread)
EOF
fi

# 4. 编译
echo "4. 编译项目..."
mkdir -p build
cd build

echo "  运行 CMake..."
cmake .. > "$SCRIPT_DIR/logs/cmake.log" 2>&1
if [ $? -ne 0 ]; then
    echo "  ❌ CMake 失败"
    echo "  查看日志: $SCRIPT_DIR/logs/cmake.log"
    exit 1
fi

echo "  编译客户端..."
make kv_client > "$SCRIPT_DIR/logs/compile.log" 2>&1
if [ $? -ne 0 ]; then
    echo "  ❌ 编译失败"
    echo "  查看日志: $SCRIPT_DIR/logs/compile.log"
    exit 1
fi

echo "✅ 编译成功！"
echo ""

# 5. 生成设置报告
REPORT_FILE="$SCRIPT_DIR/setup_report_$(date +%Y%m%d_%H%M%S).txt"
echo "=== 阶段二文件设置完成 ===" > "$REPORT_FILE"
echo "时间: $(date)" >> "$REPORT_FILE"
echo "项目目录: $PROJECT_DIR" >> "$REPORT_FILE"
echo "" >> "$REPORT_FILE"
echo "创建的文件:" >> "$REPORT_FILE"
echo "1. src/common/utils.h" >> "$REPORT_FILE"
echo "2. src/common/utils.cc" >> "$REPORT_FILE"
echo "3. src/client/connection.h" >> "$REPORT_FILE"
echo "4. src/client/connection.cc" >> "$REPORT_FILE"
echo "5. src/client/cluster_config.h" >> "$REPORT_FILE"
echo "6. src/client/cluster_config.cc" >> "$REPORT_FILE"
echo "7. src/client/router.h" >> "$REPORT_FILE"
echo "8. src/client/router.cc" >> "$REPORT_FILE"
echo "9. src/client/kv_client.h" >> "$REPORT_FILE"
echo "10. src/client/kv_client.cc" >> "$REPORT_FILE"
echo "" >> "$REPORT_FILE"
echo "备份目录: $backup_dir" >> "$REPORT_FILE"
echo "日志目录: $SCRIPT_DIR/logs/" >> "$REPORT_FILE"
echo "" >> "$REPORT_FILE"
echo "编译状态: 成功" >> "$REPORT_FILE"
echo "客户端位置: $PROJECT_DIR/build/kv_client" >> "$REPORT_FILE"

echo "✅ 文件设置完成！"
echo "📄 设置报告: $REPORT_FILE"
echo ""
echo "下一步："
echo "1. 运行测试: $SCRIPT_DIR/scripts/run_all.sh"
echo "2. 或运行单个测试:"
echo "   $SCRIPT_DIR/scripts/test_basic.sh"
echo "   $SCRIPT_DIR/scripts/test_sharding.sh"
echo ""
echo "注意：如果服务器协议不是 SET/GET/DEL，需要修改 kv_client.cc 中的 serializeRequest 函数"
EOF

# 给脚本执行权限
chmod +x "$SCRIPT_DIR/setup_files.sh"

echo "✅ 设置脚本已创建: $SCRIPT_DIR/setup_files.sh"
echo ""
echo "使用方法："
echo "1. 进入项目目录: cd ~/桌面/distributed-kv-store"
echo "2. 运行设置脚本: ./tests/phase2/setup_files.sh"
echo "3. 查看设置报告: cat tests/phase2/setup_report_*.txt"
echo "4. 运行测试: ./tests/phase2/scripts/run_all.sh"
