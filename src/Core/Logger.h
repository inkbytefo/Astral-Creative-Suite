#ifndef ASTRAL_ENGINE_LOGGER_H
#define ASTRAL_ENGINE_LOGGER_H

#include <string>
#include <iostream>
#include <fstream>
#include <mutex>
#include <chrono>
#include <fmt/core.h>

namespace AstralEngine {
    enum class LogLevel {
        Trace = 0,
        Debug = 1,
        Info = 2,
        Warn = 3,
        Error = 4,
        Fatal = 5
    };

    class Logger {
    public:
        static void Init();
        static void Shutdown();
        
        static void SetLogLevel(LogLevel level);
        static void SetLogFile(const std::string& filepath);
        
        static void Log(LogLevel level, const std::string& message);
        
        template<typename... Args>
        static void Trace(fmt::format_string<Args...> fmt, Args&&... args) {
            Log(LogLevel::Trace, fmt::format(fmt, std::forward<Args>(args)...));
        }

        template<typename... Args>
        static void Debug(fmt::format_string<Args...> fmt, Args&&... args) {
            Log(LogLevel::Debug, fmt::format(fmt, std::forward<Args>(args)...));
        }

        template<typename... Args>
        static void Info(fmt::format_string<Args...> fmt, Args&&... args) {
            Log(LogLevel::Info, fmt::format(fmt, std::forward<Args>(args)...));
        }

        template<typename... Args>
        static void Warn(fmt::format_string<Args...> fmt, Args&&... args) {
            Log(LogLevel::Warn, fmt::format(fmt, std::forward<Args>(args)...));
        }

        template<typename... Args>
        static void Error(fmt::format_string<Args...> fmt, Args&&... args) {
            Log(LogLevel::Error, fmt::format(fmt, std::forward<Args>(args)...));
        }

        template<typename... Args>
        static void Fatal(fmt::format_string<Args...> fmt, Args&&... args) {
            Log(LogLevel::Fatal, fmt::format(fmt, std::forward<Args>(args)...));
        }

    private:
        static std::string getLogLevelString(LogLevel level);
        static std::string getCurrentTimestamp();
        static void writeLog(LogLevel level, const std::string& message);
        
        static LogLevel s_logLevel;
        static std::ofstream s_logFile;
        static std::mutex s_mutex;
        static bool s_fileLoggingEnabled;
    };
}

// Convenience macros
#define AE_TRACE(...)    ::AstralEngine::Logger::Trace(__VA_ARGS__)
#define AE_DEBUG(...)    ::AstralEngine::Logger::Debug(__VA_ARGS__)
#define AE_INFO(...)     ::AstralEngine::Logger::Info(__VA_ARGS__)
#define AE_WARN(...)     ::AstralEngine::Logger::Warn(__VA_ARGS__)
#define AE_ERROR(...)    ::AstralEngine::Logger::Error(__VA_ARGS__)
#define AE_FATAL(...)    ::AstralEngine::Logger::Fatal(__VA_ARGS__)

#endif // ASTRAL_ENGINE_LOGGER_H
