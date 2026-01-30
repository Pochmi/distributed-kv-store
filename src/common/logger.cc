// src/common/logger.cc
#include "logger.h"

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

void Logger::set_level(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    level_ = level;
}

void Logger::log(const std::string& level_str, const std::string& message, 
                const std::string& file, int line) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string file_info = file.empty() ? "" : " [" + file + ":" + std::to_string(line) + "]";
    std::cout << "[" << level_str << "]" << file_info << " " << message << std::endl;
}

void Logger::debug(const std::string& message, const std::string& file, int line) {
    if (level_ <= DEBUG) {
        log("DEBUG", message, file, line);
    }
}

void Logger::info(const std::string& message, const std::string& file, int line) {
    if (level_ <= INFO) {
        log("INFO", message, file, line);
    }
}

void Logger::warning(const std::string& message, const std::string& file, int line) {
    if (level_ <= WARNING) {
        log("WARNING", message, file, line);
    }
}

void Logger::error(const std::string& message, const std::string& file, int line) {
    if (level_ <= ERROR) {
        log("ERROR", message, file, line);
    }
}
