#include <iostream>
#include <string>
#include "cluster/admin_client.h"
#include "common/logger.h"

int main(int argc, char* argv[]) {
    Logger::instance().setLevel(LOG_INFO);
    
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <master-address>" << std::endl;
        std::cout << "Example: " << argv[0] << " 127.0.0.1:6380" << std::endl;
        return 1;
    }
    
    std::string master_addr = argv[1];
    std::cout << "Connecting to master at " << master_addr << std::endl;
    
    AdminCommandClient client(master_addr);
    
    // 简单的命令行界面
    std::string command;
    while (true) {
        std::cout << "admin> ";
        std::getline(std::cin, command);
        
        if (command == "quit" || command == "exit") {
            break;
        } else if (command == "help") {
            std::cout << "Available commands:" << std::endl;
            std::cout << "  list      - List cluster nodes" << std::endl;
            std::cout << "  stats     - Show cluster statistics" << std::endl;
            std::cout << "  failover  - Manual failover" << std::endl;
            std::cout << "  promote <slave_id> - Promote slave to master" << std::endl;
            std::cout << "  demote <master_id> - Demote master to slave" << std::endl;
            std::cout << "  quit      - Exit program" << std::endl;
        } else if (command == "list") {
            auto nodes = client.listNodes();
            std::cout << "Found " << nodes.size() << " nodes" << std::endl;
        } else if (command == "stats") {
            auto stats = client.getStats();
            std::cout << "Cluster statistics:" << std::endl;
            std::cout << "  Total nodes: " << stats.total_nodes << std::endl;
            std::cout << "  Alive nodes: " << stats.alive_nodes << std::endl;
            std::cout << "  Dead nodes: " << stats.dead_nodes << std::endl;
            std::cout << "  Master: " << stats.master_id << std::endl;
            std::cout << "  Slaves: ";
            for (const auto& slave : stats.slave_ids) {
                std::cout << slave << " ";
            }
            std::cout << std::endl;
        } else if (command == "failover") {
            if (client.initiateFailover()) {
                std::cout << "Failover initiated successfully" << std::endl;
            } else {
                std::cout << "Failover failed: " << client.getLastError() << std::endl;
            }
        } else if (command.substr(0, 7) == "promote") {
            if (command.length() > 8) {
                std::string slave_id = command.substr(8);
                if (client.promoteSlave(slave_id)) {
                    std::cout << "Promoted " << slave_id << std::endl;
                } else {
                    std::cout << "Failed to promote: " << client.getLastError() << std::endl;
                }
            } else {
                std::cout << "Usage: promote <slave_id>" << std::endl;
            }
        } else if (command.substr(0, 6) == "demote") {
            if (command.length() > 7) {
                std::string master_id = command.substr(7);
                if (client.demoteMaster(master_id)) {
                    std::cout << "Demoted " << master_id << std::endl;
                } else {
                    std::cout << "Failed to demote: " << client.getLastError() << std::endl;
                }
            } else {
                std::cout << "Usage: demote <master_id>" << std::endl;
            }
        } else {
            std::cout << "Unknown command. Type 'help' for available commands." << std::endl;
        }
    }
    
    return 0;
}
