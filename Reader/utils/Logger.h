#pragma once
#include "common.h"
#include <thread>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <memory>

#if ENABLE_LOGGER

class Logger
{
public:
    enum class Level
    {
        ERRO,
        WARN,
        INFO,
        DEBU
    };

public:
    static Logger& instance();
    void printf(Level level, const char* format, ...);

private:
    Logger();
    ~Logger();
    void flush();
    void writer();
    void backup();
    std::string timeStr();
    std::wstring logName();
    std::wstring logBakName();

private:
    std::thread m_thread;
    std::queue<std::string> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    bool m_stop;
    FILE* m_fp;
    bool m_isDebugger;
};

#define lgerror(fmt, ...)  Logger::instance().printf(Logger::Level::ERRO, fmt, ##__VA_ARGS__);
#define lgwarn(fmt, ...)   Logger::instance().printf(Logger::Level::WARN, fmt, ##__VA_ARGS__);
#define lginfo(fmt, ...)   Logger::instance().printf(Logger::Level::INFO, fmt, ##__VA_ARGS__);
#define lgdebug(fmt, ...)  Logger::instance().printf(Logger::Level::DEBU, fmt, ##__VA_ARGS__);

extern void lgprint(int level, const char* text);

#else

#define lgerror(fmt, ...)
#define lgwarn(fmt, ...)
#define lginfo(fmt, ...)
#define lgdebug(fmt, ...)

extern void lgprint(int level, const char* text);

#endif