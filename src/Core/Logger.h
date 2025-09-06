// Logger.h

#pragma once

#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <cstdarg> // printf stili fonksiyonlar için gerekli

namespace Astral {
    class Logger {
    private:
        // Mesajları formatlamak için özel bir yardımcı fonksiyon
        static void Log(const char* level, const char* color, const char* format, va_list args) {
            // Güvenli bir şekilde formatlanmış string'in boyutunu hesapla
            va_list args_copy;
            va_copy(args_copy, args);
            int size = vsnprintf(nullptr, 0, format, args_copy);
            va_end(args_copy);

            if (size >= 0) {
                // Hafızadan formatlanmış mesaj için yer ayır
                std::vector<char> buffer(size + 1);
                vsnprintf(buffer.data(), buffer.size(), format, args);
                
                // Renkli çıktıyı konsola yazdır
                std::cout << color << level << "\033[0m " << buffer.data() << std::endl;
            }
        }

    public:
        static void Init() {
            std::cout << "Logger Başlatıldı." << std::endl;
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
    };
}

// Makrolar aynı kalabilir, kullanımları değişecek
#define ASTRAL_LOG_INFO(...)  ::Astral::Logger::LogInfo(__VA_ARGS__)
#define ASTRAL_LOG_WARN(...)  ::Astral::Logger::LogWarn(__VA_ARGS__)
#define ASTRAL_LOG_ERROR(...) ::Astral::Logger::LogError(__VA_ARGS__)
