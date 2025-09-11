#include "Logger.h"
#include <iostream>
#include <string>

namespace AstralEngine {
    void Logger::Init() {
        // Initialize logger (simple console output for now)
        std::cout << "Logger initialized" << std::endl;
    }
    
    void Logger::Shutdown() {
        // Shutdown logger
        std::cout << "Logger shutdown" << std::endl;
    }
    
    void Logger::Log(LogLevel level, const std::string& message) {
        // Simple console output for now
        std::cout << "[" << getLogLevelString(level) << "] " << message << std::endl;
    }
    
    std::string Logger::getLogLevelString(LogLevel level) {
        switch (level) {
            case LogLevel::Trace: return "TRACE";
            case LogLevel::Debug: return "DEBUG";
            case LogLevel::Info:  return "INFO";
            case LogLevel::Warn:  return "WARN";
            case LogLevel::Error: return "ERROR";
            case LogLevel::Fatal: return "FATAL";
            default:              return "UNKNOWN";
        }
    }
}