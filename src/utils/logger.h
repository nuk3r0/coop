#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <cstdarg>

enum class LogLevel { Debug, Info, Warning, Error };

class Logger {
public:
    static Logger& Get() {
        static Logger l;
        return l;
    }
    bool Initialize(const std::string& filePath);
    void Shutdown();
    void Log(LogLevel level, const char* fmt, ...);
private:
    std::ofstream m_file;
    std::mutex m_mutex;
    bool m_initialized = false;
};

#define LOG_DEBUG(...) Logger::Get().Log(LogLevel::Debug, __VA_ARGS__)
#define LOG_INFO(...)  Logger::Get().Log(LogLevel::Info, __VA_ARGS__)
#define LOG_WARNING(...) Logger::Get().Log(LogLevel::Warning, __VA_ARGS__)
#define LOG_ERROR(...) Logger::Get().Log(LogLevel::Error, __VA_ARGS__)
