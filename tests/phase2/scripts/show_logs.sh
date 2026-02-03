#!/bin/bash

# ============================================
# 日志查看工具
# 方便查看测试过程中的日志
# ============================================

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PHASE2_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"
PROJECT_DIR="$( cd "$PHASE2_DIR/../.." && pwd )"
LOGS_DIR="$PHASE2_DIR/logs"
SERVERS_DIR="$LOGS_DIR/servers"

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# 显示菜单
show_menu() {
    clear
    echo -e "${CYAN}╔══════════════════════════════════════════╗${NC}"
    echo -e "${CYAN}║     分布式KV存储 - 阶段二日志查看器       ║${NC}"
    echo -e "${CYAN}╚══════════════════════════════════════════╝${NC}"
    echo ""
    echo -e "${YELLOW}请选择要查看的日志:${NC}"
    echo ""
    echo -e "  ${GREEN}1${NC}. 查看服务器1 (shard-1, 端口 6381)"
    echo -e "  ${GREEN}2${NC}. 查看服务器2 (shard-2, 端口 6382)"
    echo -e "  ${GREEN}3${NC}. 查看服务器3 (shard-3, 端口 6383)"
    echo -e "  ${GREEN}4${NC}. 查看所有服务器日志"
    echo -e "  ${GREEN}5${NC}. 查看CMake构建日志"
    echo -e "  ${GREEN}6${NC}. 查看Make编译日志"
    echo -e "  ${GREEN}7${NC}. 查看路由分布统计"
    echo -e "  ${GREEN}8${NC}. 查看最近错误日志"
    echo -e "  ${GREEN}9${NC}. 实时监控所有日志"
    echo -e "  ${GREEN}0${NC}. 退出"
    echo ""
}

# 查看单个服务器日志
view_server_log() {
    local port=$1
    local node_name=$2
    local log_file="$SERVERS_DIR/server_$port.log"
    
    if [ ! -f "$log_file" ]; then
        echo -e "${RED}❌ 日志文件不存在: $log_file${NC}"
        read -p "按回车键继续..."
        return
    fi
    
    clear
    echo -e "${CYAN}════════════════════════════════════════════${NC}"
    echo -e "${CYAN}  服务器 $node_name (127.0.0.1:$port) 日志   ${NC}"
    echo -e "${CYAN}════════════════════════════════════════════${NC}"
    echo ""
    
    # 显示最后50行
    tail -n 50 "$log_file"
    
    echo ""
    echo -e "${CYAN}════════════════════════════════════════════${NC}"
    echo -e "文件: $log_file"
    echo -e "大小: $(du -h "$log_file" | cut -f1)"
    echo -e "最后修改: $(stat -c %y "$log_file")"
    echo -e "${CYAN}════════════════════════════════════════════${NC}"
    
    echo ""
    echo -e "${YELLOW}操作选项:${NC}"
    echo -e "  [f] 查看完整日志"
    echo -e "  [t] 实时跟踪日志"
    echo -e "  [e] 查看错误信息"
    echo -e "  [r] 查看路由信息"
    echo -e "  [b] 返回主菜单"
    
    read -p "选择操作: " choice
    
    case $choice in
        f|F)
            less "$log_file"
            ;;
        t|T)
            echo -e "${GREEN}开始实时跟踪日志 (Ctrl+C 退出)...${NC}"
            tail -f "$log_file"
            ;;
        e|E)
            echo -e "${RED}=== 错误信息 ===${NC}"
            grep -i "error\|fail\|exception\|崩溃" "$log_file" | tail -20
            read -p "按回车键继续..."
            ;;
        r|R)
            echo -e "${BLUE}=== 路由信息 ===${NC}"
            grep -i "routed\|route\|key.*to\|处理键" "$log_file" | tail -20
            read -p "按回车键继续..."
            ;;
    esac
}

# 查看路由分布
view_routing_stats() {
    clear
    echo -e "${CYAN}════════════════════════════════════════════${NC}"
    echo -e "${CYAN}         路由分布统计                      ${NC}"
    echo -e "${CYAN}════════════════════════════════════════════${NC}"
    echo ""
    
    total_keys=0
    for port in 6381 6382 6383; do
        log_file="$SERVERS_DIR/server_$port.log"
        if [ -f "$log_file" ]; then
            # 统计处理的键数量
            key_count=$(grep -c "Processing key\|处理键\|key.*received" "$log_file" 2>/dev/null || echo "0")
            echo -e "服务器 ${GREEN}$port${NC}: ${YELLOW}$key_count${NC} 个键"
            total_keys=$((total_keys + key_count))
            
            # 显示最近处理的5个键
            echo -n "  最近键: "
            grep "Processing key\|处理键" "$log_file" | tail -3 | sed 's/.*key //' | tr '\n' ',' | sed 's/,$//'
            echo ""
        else
            echo -e "服务器 ${RED}$port${NC}: 日志文件不存在"
        fi
    done
    
    echo ""
    echo -e "总处理键数: ${YELLOW}$total_keys${NC}"
    
    if [ $total_keys -gt 0 ]; then
        for port in 6381 6382 6383; do
            log_file="$SERVERS_DIR/server_$port.log"
            if [ -f "$log_file" ]; then
                key_count=$(grep -c "Processing key\|处理键" "$log_file" 2>/dev/null || echo "0")
                percentage=$((key_count * 100 / total_keys))
                echo -e "服务器 $port 分布: ${BLUE}$percentage%${NC}"
            fi
        done
    fi
    
    echo ""
    read -p "按回车键返回主菜单..."
}

# 主循环
while true; do
    show_menu
    read -p "请输入选项 (0-9): " choice
    
    case $choice in
        1)
            view_server_log 6381 "shard-1"
            ;;
        2)
            view_server_log 6382 "shard-2"
            ;;
        3)
            view_server_log 6383 "shard-3"
            ;;
        4)
            clear
            echo -e "${CYAN}════════════════════════════════════════════${NC}"
            echo -e "${CYAN}         所有服务器日志 (最后20行)          ${NC}"
            echo -e "${CYAN}════════════════════════════════════════════${NC}"
            echo ""
            for port in 6381 6382 6383; do
                log_file="$SERVERS_DIR/server_$port.log"
                if [ -f "$log_file" ]; then
                    echo -e "${GREEN}=== 服务器 $port ===${NC}"
                    tail -n 20 "$log_file"
                    echo ""
                fi
            done
            read -p "按回车键继续..."
            ;;
        5)
            clear
            echo -e "${CYAN}════════════════════════════════════════════${NC}"
            echo -e "${CYAN}           CMake构建日志                   ${NC}"
            echo -e "${CYAN}════════════════════════════════════════════${NC}"
            echo ""
            if [ -f "$LOGS_DIR/cmake.log" ]; then
                tail -n 50 "$LOGS_DIR/cmake.log"
            else
                echo -e "${RED}CMake日志文件不存在${NC}"
            fi
            read -p "按回车键继续..."
            ;;
        6)
            clear
            echo -e "${CYAN}════════════════════════════════════════════${NC}"
            echo -e "${CYAN}           Make编译日志                    ${NC}"
            echo -e "${CYAN}════════════════════════════════════════════${NC}"
            echo ""
            if [ -f "$LOGS_DIR/make.log" ]; then
                tail -n 50 "$LOGS_DIR/make.log"
            else
                echo -e "${RED}Make日志文件不存在${NC}"
            fi
            read -p "按回车键继续..."
            ;;
        7)
            view_routing_stats
            ;;
        8)
            clear
            echo -e "${CYAN}════════════════════════════════════════════${NC}"
            echo -e "${CYAN}           最近错误日志                    ${NC}"
            echo -e "${CYAN}════════════════════════════════════════════${NC}"
            echo ""
            for port in 6381 6382 6383; do
                log_file="$SERVERS_DIR/server_$port.log"
                if [ -f "$log_file" ]; then
                    errors=$(grep -i "error\|fail\|exception" "$log_file" | tail -5)
                    if [ -n "$errors" ]; then
                        echo -e "${RED}=== 服务器 $port 错误 ===${NC}"
                        echo "$errors"
                        echo ""
                    fi
                fi
            done
            read -p "按回车键继续..."
            ;;
        9)
            clear
            echo -e "${CYAN}════════════════════════════════════════════${NC}"
            echo -e "${CYAN}     实时监控所有日志 (Ctrl+C 退出)        ${NC}"
            echo -e "${CYAN}════════════════════════════════════════════${NC}"
            echo ""
            echo -e "${YELLOW}开始监控日志...${NC}"
            echo ""
            multitail \
                -cT ansi -f \
                -l "tail -f $SERVERS_DIR/server_6381.log" \
                -l "tail -f $SERVERS_DIR/server_6382.log" \
                -l "tail -f $SERVERS_DIR/server_6383.log"
            ;;
        0)
            echo -e "${GREEN}退出日志查看器。${NC}"
            exit 0
            ;;
        *)
            echo -e "${RED}无效选项，请重新选择${NC}"
            sleep 1
            ;;
    esac
done
