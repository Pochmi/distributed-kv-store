bool ClusterConfig::parseJsonConfig(const std::string& json_str) {
    std::lock_guard<std::mutex> lock(mutex_);
    nodes_.clear();
    
    // 查找 "nodes" 数组（可能在一级，也可能在 "cluster" 下）
    size_t nodes_pos = json_str.find("\"nodes\"");
    if (nodes_pos == std::string::npos) return false;
    
    size_t array_start = json_str.find('[', nodes_pos);
    size_t array_end = json_str.find(']', array_start);
    if (array_start == std::string::npos || array_end == std::string::npos) return false;
    
    std::string nodes_str = json_str.substr(array_start + 1, array_end - array_start - 1);
    
    size_t pos = 0;
    while (true) {
        size_t obj_start = nodes_str.find('{', pos);
        if (obj_start == std::string::npos) break;
        
        size_t obj_end = nodes_str.find('}', obj_start);
        if (obj_end == std::string::npos) break;
        
        std::string obj = nodes_str.substr(obj_start, obj_end - obj_start + 1);
        
        auto extract = [&](const std::string& field) -> std::string {
            size_t fpos = obj.find("\"" + field + "\"");
            if (fpos == std::string::npos) return "";
            size_t colon = obj.find(':', fpos);
            size_t q1 = obj.find('"', colon);
            size_t q2 = obj.find('"', q1 + 1);
            if (q1 == std::string::npos || q2 == std::string::npos) return "";
            return obj.substr(q1 + 1, q2 - q1 - 1);
        };
        
        NodeInfo node;
        node.id = extract("id");
        node.host = extract("host");
        std::string port_str = extract("port");
        if (!port_str.empty()) node.port = std::stoi(port_str);
        node.role = extract("role");
        if (node.role.empty()) node.role = "master";
        
        if (!node.id.empty() && !node.host.empty() && node.port > 0) {
            nodes_.push_back(node);
        }
        
        pos = obj_end + 1;
    }
    
    return !nodes_.empty();
}
