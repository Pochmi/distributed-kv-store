#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>

namespace utils {
    std::vector<std::string> Split(const std::string& str, char delimiter);
    std::string Trim(const std::string& str);
}

#endif
