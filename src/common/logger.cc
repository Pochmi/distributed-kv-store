#include "logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

Logger::Logger() : level_(LOG_INFO) {
}

Logger::~Logger() {
}

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

void Logger::setLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    level_ = level;
}

std::string Logger::getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string Logger::levelToString(LogLevel level) {
    switch(level) {
        case LOG_DEBUG:   return "DEBUG";
        case LOG_INFO:    return "INFO";
        case LOG_WARNING: return "WARNING";
        case LOG_ERROR:   return "ERROR";
        default:          return "UNKNOWN";
    }
}

void Logger::debug(const std::string& msg, const std::string& file, int line) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (level_ <= LOG_DEBUG) {
        std::cout << "[" << getCurrentTime() << "] "
                  << "[" << levelToString(LOG_DEBUG) << "] "
                  << msg 
                  << " (" << file << ":" << line << ")"
                  << std::endl;
    }
}

void Logger::info(const std::string& msg, const std::string& file, int line) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (level_ <= LOG_INFO) {
        std::cout << "[" << getCurrentTime() << "] "
                  << "[" << levelToString(LOG_INFO) << "] "
                  << msg 
                  << " (" << file << ":" << line << ")"
                  << std::endl;
    }
}

void Logger::warning(const std::string& msg, const std::string& file, int line) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (level_ <= LOG_WARNING) {
        std::cout << "[" << getCurrentTime() << "] "
                  << "[" << levelToString(LOG_WARNING) << "] "
                  << msg 
                  << " (" << file << ":" << line << ")"
                  << std::endl;
    }
}

void Logger::error(const std::string& msg, const std::string& file, int line) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (level_ <= LOG_ERROR) {
        std::cerr << "[" << getCurrentTime() << "] "
                  << "[" << levelToString(LOG_ERROR) << "] "
                  << msg 
                  << " (" << file << ":" << line << ")"
                  << std::endl;
    }
}
