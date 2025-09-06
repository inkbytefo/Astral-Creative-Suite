#pragma once

#include <string>

namespace Astral {
    class ConfigManager {
    public:
        ConfigManager();
        ~ConfigManager();

        bool LoadFromFile(const std::string& filepath);
        
        // Örnek getter fonksiyonları
        std::string GetString(const std::string& key, const std::string& defaultValue = "");
        int GetInt(const std::string& key, int defaultValue = 0);
    };
}
