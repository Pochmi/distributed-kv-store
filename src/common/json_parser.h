#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include <string>
#include <vector>
#include <map>

namespace JsonParser {
    // 简单的JSON解析器（用于解析配置）
    std::map<std::string, std::string> parseSimpleJson(const std::string& json_str);
    
    // 从文件加载JSON
    std::string loadJsonFile(const std::string& filename);
    
    // 解析集群配置
    bool parseClusterConfig(const std::string& json_str, 
                           std::vector<NodeInfo>& nodes);
}

#endif
