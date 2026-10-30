#include "Logger.h"
#include "Utils.h"
#include "common.h"
#include <cstdarg>
#include <chrono>
#include <iomanip>
#include <sstream>

#if ENABLE_LOGGER

Logger& Logger::instance()
{
    static Logger _inst;
    return _inst;
}

void Logger::printf(Level level, const char* fmt, ...)
{
    std::string text;
    text.append("{");
    text.append(timeStr());
    text.append("}-");
    
    switch (level)
    {
    case Level::ERRO:
        text.append("[Erro] ");
        break;
    case Level::WARN:
        text.append("[Warn] ");
        break;
    case Level::INFO:
        text.append("[Info] ");
        break;
    case Level::DEBU:
        text.append("[Debu] ");
        break;

    }

    char buffer[4096] = { 0 };
    va_list ap;
    va_start(ap, fmt);
    std::vsnprintf(buffer, sizeof(buffer), fmt, ap);
    va_end(ap);
    text.append(buffer);
    if (text.empty() || text.back() != '\n') text.append("\n");

    if (m_isDebugger)
    {
        OutputDebugStringA(text.c_str());
        return;
    }

    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_queue.push(std::move(text));
    }
    m_cv.notify_one();
}

Logger::Logger()
    : m_stop(false)
    , m_fp(nullptr)
{
    m_isDebugger = IsDebuggerPresent();
    if (!m_isDebugger)
    {
        m_thread = std::thread(&Logger::writer, this);
    }
}

Logger::~Logger()
{
    flush();
    if (m_fp)
    {
        fclose(m_fp);
        m_fp = nullptr;
    }
}

void Logger::flush()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stop = true;
        m_cv.notify_one();
    }
    if (m_thread.joinable())
    {
        m_thread.join();
    }
}

void Logger::writer()
{
    while (true)
    {
        std::string text;

        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this] { return !m_queue.empty() || m_stop; });
            if (m_stop && m_queue.empty())
            {
                break;
            }

            if (!m_queue.empty())
            {
                text = std::move(m_queue.front());
                m_queue.pop();
            }
        }

        if (!text.empty())
        {
            backup();
            if (m_fp)
            {
                fwrite(text.c_str(), 1, text.size(), m_fp);
            }
        }
    }
}

void Logger::backup()
{
    if (m_fp)
    {
        int size = ftell(m_fp);
        if (size > 32 * 1024 * 1024)
        {
            fclose(m_fp);
            m_fp = nullptr;
            Utils::renameFile(logName(), logBakName());
        }
    }
    if (!m_fp)
    {
        m_fp = _wfopen(logName().c_str(), L"ab");
    }
}

std::string Logger::timeStr()
{
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_time), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

std::wstring Logger::logName()
{
    return Utils::appPath() + LOG_FILE_NAME;
}

std::wstring Logger::logBakName()
{
    return Utils::appPath() + LOG_BAK_FILE_NAME;
}
#endif

void lgprint(int level, const char* text)
{
#if ENABLE_LOGGER
    Logger::instance().printf((Logger::Level)level, "%s", text);
#endif
}