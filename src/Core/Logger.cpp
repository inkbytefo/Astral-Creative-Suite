#include "Logger.h"
#include <iostream>
#include <string>
#include <iomanip>
#include <sstream>

namespace AstralEngine {
    // Static member definitions
    LogLevel Logger::s_logLevel = LogLevel::Info;
    std::ofstream Logger::s_logFile;
    std::mutex Logger::s_mutex;
    bool Logger::s_fileLoggingEnabled = false;
    
    void Logger::Init() {
        // Initialize logger
        std::cout << "[" << getCurrentTimestamp() << "] [INFO] Logger initialized" << std::endl;
    }
    
    void Logger::Shutdown() {
        // Shutdown logger
        std::lock_guard<std::mutex> lock(s_mutex);
        if (s_fileLoggingEnabled && s_logFile.is_open()) {
            s_logFile << "[" << getCurrentTimestamp() << "] [INFO] Logger shutdown" << std::endl;
            s_logFile.close();
        }
        std::cout << "[" << getCurrentTimestamp() << "] [INFO] Logger shutdown" << std::endl;
    }
    
    void Logger::SetLogLevel(LogLevel level) {
        s_logLevel = level;
    }
    
    void Logger::SetLogFile(const std::string& filepath) {
        std::lock_guard<std::mutex> lock(s_mutex);
        if (s_logFile.is_open()) {
            s_logFile.close();
        }
        
        s_logFile.open(filepath, std::ios::out | std::ios::app);
        s_fileLoggingEnabled = s_logFile.is_open();
    }
    
    void Logger::Log(LogLevel level, const std::string& message) {
        if (level >= s_logLevel) {
            writeLog(level, message);
        }
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
    
    std::string Logger::getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 100;
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }
    
    void Logger::writeLog(LogLevel level, const std::string& message) {
        std::lock_guard<std::mutex> lock(s_mutex);
        std::string logEntry = "[" + getCurrentTimestamp() + "] [" + getLogLevelString(level) + "] " + message;
        
        // Write to console
        std::cout << logEntry << std::endl;
        
        // Write to file if enabled
        if (s_fileLoggingEnabled && s_logFile.is_open()) {
            s_logFile << logEntry << std::endl;
            s_logFile.flush();
        }
    }
}
