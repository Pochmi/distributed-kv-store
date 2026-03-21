#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <mutex>

// 如果定义了 DEBUG 宏，先取消
#ifdef DEBUG
#undef DEBUG
#endif

enum LogLevel {
    LOG_DEBUG = 0,
    LOG_INFO = 1,
    LOG_WARNING = 2,
    LOG_ERROR = 3
};

class Logger {
public:
    static Logger& instance();
    
    void setLevel(LogLevel level);
    
    void debug(const std::string& msg, const std::string& file = "", int line = 0);
    void info(const std::string& msg, const std::string& file = "", int line = 0);
    void warning(const std::string& msg, const std::string& file = "", int line = 0);
    void error(const std::string& msg, const std::string& file = "", int line = 0);
    
private:
    Logger();
    ~Logger();
    
    std::string getCurrentTime();
    std::string levelToString(LogLevel level);
    
    LogLevel level_;
    std::mutex mutex_;
};

// 方便使用的宏
#define LOG_DEBUG(msg) Logger::instance().debug(msg, __FILE__, __LINE__)
#define LOG_INFO(msg) Logger::instance().info(msg, __FILE__, __LINE__)
#define LOG_WARNING(msg) Logger::instance().warning(msg, __FILE__, __LINE__)
#define LOG_ERROR(msg) Logger::instance().error(msg, __FILE__, __LINE__)

#endif
