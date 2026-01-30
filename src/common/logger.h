// src/common/logger.h
#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <iostream>
#include <mutex>

enum LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3
};

class Logger {
public:
    static Logger& instance();
    
    void set_level(LogLevel level);
    LogLevel get_level() const;
    
    void debug(const std::string& message, const std::string& file = "", int line = 0);
    void info(const std::string& message, const std::string& file = "", int line = 0);
    void warning(const std::string& message, const std::string& file = "", int line = 0);
    void error(const std::string& message, const std::string& file = "", int line = 0);

private:
    Logger() : level_(INFO) {}
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    void log(const std::string& level_str, const std::string& message, 
             const std::string& file, int line);
    
    LogLevel level_;
    std::mutex mutex_;
};

// 宏定义便于使用
#define LOG_DEBUG(msg) Logger::instance().debug(msg, __FILE__, __LINE__)
#define LOG_INFO(msg) Logger::instance().info(msg, __FILE__, __LINE__)
#define LOG_WARNING(msg) Logger::instance().warning(msg, __FILE__, __LINE__)
#define LOG_ERROR(msg) Logger::instance().error(msg, __FILE__, __LINE__)

#endif // LOGGER_H
