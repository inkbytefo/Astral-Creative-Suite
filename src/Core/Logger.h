// Logger.h

#pragma once

#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <cstdarg> // printf stili fonksiyonlar için gerekli
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <thread>
#include <cstring>

namespace AstralEngine {
    class Logger {
    private:
        static std::ofstream logFile;
        static bool debugMode;
        static std::string logFileName;
        
        // Mesajları formatlamak için özel bir yardımcı fonksiyon
        static void Log(const char* level, const char* color, const char* format, va_list args) {
            // Log dosyasına yaz
            if (logFile.is_open()) {
                // Timestamp oluştur
                auto now = std::chrono::system_clock::now();
                auto time_t_now = std::chrono::system_clock::to_time_t(now);
                std::tm tm_now;
                localtime_s(&tm_now, &time_t_now);
                
                // Timestamp'ı formatlayarak log dosyasına yaz
                logFile << "[" << std::put_time(&tm_now, "%Y-%m-%d %H:%M:%S") << "] " << level << " ";
                
                // Güvenli bir şekilde formatlanmış string'in boyutunu hesapla
                va_list args_copy;
                va_copy(args_copy, args);
                int size = vsnprintf(nullptr, 0, format, args_copy);
                va_end(args_copy);

                if (size >= 0) {
                    // Hafızadan formatlanmış mesaj için yer ayır
                    std::vector<char> buffer(size + 1);
                    vsnprintf(buffer.data(), buffer.size(), format, args);
                    
                    // Log dosyasına yaz
                    logFile << buffer.data() << std::endl;
                    
                    // Debug moddaysa konsola da yaz
                    if (debugMode) {
                        std::cout << color << level << "\033[0m " << buffer.data() << std::endl;
                    }
                }
            } else {
                // Log dosyası açık değilse sadece konsola yaz
                va_list args_copy;
                va_copy(args_copy, args);
                int size = vsnprintf(nullptr, 0, format, args_copy);
                va_end(args_copy);

                if (size >= 0) {
                    std::vector<char> buffer(size + 1);
                    vsnprintf(buffer.data(), buffer.size(), format, args);
                    std::cout << color << level << "\033[0m " << buffer.data() << std::endl;
                }
            }
            
            // Log dosyasını flush et önemli hatalarda
            if (strcmp(level, "[ERROR]") == 0) {
                logFile.flush();
            }
        }

    public:
        static void Init() {
            debugMode = true; // Debug mode varsayılan olarak açık
            std::string timestamp = GetCurrentTimestamp();
            logFileName = "astral_engine_" + timestamp + ".log";
            
            // Log dosyasını aç
            logFile.open(logFileName);
            if (logFile.is_open()) {
                std::cout << "Logger Başlatıldı. Log dosyası: " << logFileName << std::endl;
                logFile << "=== AstralEngine Log Başlatıldı ===" << std::endl;
                logFile << "Timestamp: " << timestamp << std::endl;
                logFile << "=====================================" << std::endl << std::endl;
            } else {
                std::cout << "Uyarı: Log dosyası açılamadı!" << std::endl;
            }
        }
        
        static std::string GetCurrentTimestamp() {
            auto now = std::chrono::system_clock::now();
            auto time_t_now = std::chrono::system_clock::to_time_t(now);
            std::tm tm_now;
            localtime_s(&tm_now, &time_t_now);
            
            std::stringstream ss;
            ss << std::put_time(&tm_now, "%Y%m%d_%H%M%S");
            return ss.str();
        }
        
        static void SetDebugEnabled(bool enabled) {
            debugMode = enabled;
        }
        
        static void Shutdown() {
            if (logFile.is_open()) {
                logFile << std::endl;
                logFile << "=== AstralEngine Log Kapatıldı ===" << std::endl;
                logFile.close();
                std::cout << "Logger Kapatıldı." << std::endl;
            }
        }

        // Args (variadic arguments) kabul eden yeni fonksiyonlar
        static void LogInfo(const char* format, ...) {
            va_list args;
            va_start(args, format);
            Log("[INFO]", "\033[32m", format, args); // Yeşil renk
            va_end(args);
        }

        static void LogWarn(const char* format, ...) {
            va_list args;
            va_start(args, format);
            Log("[WARN]", "\033[33m", format, args); // Sarı renk
            va_end(args);
        }
        
        static void LogError(const char* format, ...) {
            va_list args;
            va_start(args, format);
            Log("[ERROR]", "\033[31m", format, args); // Kırmızı renk
            va_end(args);
        }
        
        // Debug seviyesi log'lar (sadece debug moddaysa)
        static void LogDebug(const char* format, ...) {
            if (debugMode) {
                va_list args;
                va_start(args, format);
                Log("[DEBUG]", "\033[36m", format, args); // Cyan renk
                va_end(args);
            }
        }
        
        // Critical seviyesi log'lar (her zaman yazılır)
        static void LogCritical(const char* format, ...) {
            va_list args;
            va_start(args, format);
            Log("[CRITICAL]", "\033[35m", format, args); // Magenta renk
            va_end(args);
        }
    };
}

// Makrolar aynı kalabilir, kullanımları değişecek
#define AE_INFO(...)  ::AstralEngine::Logger::LogInfo(__VA_ARGS__)
#define AE_WARN(...)  ::AstralEngine::Logger::LogWarn(__VA_ARGS__)
#define AE_ERROR(...) ::AstralEngine::Logger::LogError(__VA_ARGS__)
#define AE_DEBUG(...) ::AstralEngine::Logger::LogDebug(__VA_ARGS__)
#define AE_CRITICAL(...) ::AstralEngine::Logger::LogCritical(__VA_ARGS__)
