#pragma once

#include <string>
#include <iostream>
#include <cstdarg>

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
        
        static void Log(LogLevel level, const std::string& message);
        
        static void Trace(const std::string& message) { Log(LogLevel::Trace, message); }
        static void Debug(const std::string& message) { Log(LogLevel::Debug, message); }
        static void Info(const std::string& message) { Log(LogLevel::Info, message); }
        static void Warn(const std::string& message) { Log(LogLevel::Warn, message); }
        static void Error(const std::string& message) { Log(LogLevel::Error, message); }
        static void Fatal(const std::string& message) { Log(LogLevel::Fatal, message); }

    private:
        static std::string getLogLevelString(LogLevel level);
    };
}

// Convenience macros
#define AE_TRACE(...)    ::AstralEngine::Logger::Trace(__VA_ARGS__)
#define AE_DEBUG(...)    ::AstralEngine::Logger::Debug(__VA_ARGS__)
#define AE_INFO(...)     ::AstralEngine::Logger::Info(__VA_ARGS__)
#define AE_WARN(...)     ::AstralEngine::Logger::Warn(__VA_ARGS__)
#define AE_ERROR(...)    ::AstralEngine::Logger::Error(__VA_ARGS__)
#define AE_FATAL(...)    ::AstralEngine::Logger::Fatal(__VA_ARGS__)