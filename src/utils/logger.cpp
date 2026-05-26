#include "logger.h"
#include <cstdio>   // std::vsnprintf
#include <cstdarg>  // va_list
#include <ctime>    // std::time, std::localtime_s, std::strftime
#include <chrono>   // std::chrono

bool Logger::Initialize(const std::string& filePath) {
    m_file.open(filePath, std::ios::out | std::ios::trunc);
    m_initialized = m_file.is_open();
    return m_initialized;
}

void Logger::Shutdown() {
    if (m_initialized) {
        m_file.close();
        m_initialized = false;
    }
}

void Logger::Log(LogLevel level, const char* fmt, ...) {
    if (!m_initialized) return;
    char buf[2048];
    std::va_list args;
    va_start(args, fmt);
    std::vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    tm timeinfo{};
    localtime_s(&timeinfo, &t);

    const char* lvl = "DBG";
    switch (level) {
        case LogLevel::Info: lvl = "INF"; break;
        case LogLevel::Warning: lvl = "WRN"; break;
        case LogLevel::Error: lvl = "ERR"; break;
        default: break;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    char timebuf[32];
    std::strftime(timebuf, sizeof(timebuf), "%H:%M:%S", &timeinfo);
    m_file << "[" << timebuf << "][" << lvl << "] " << buf << "\n";
    m_file.flush();
}
