#include "ConfigManager.h"
#include "Logger.h"

namespace Astral {
    ConfigManager::ConfigManager() {
        ASTRAL_LOG_INFO("ConfigManager oluşturuldu.");
    }

    ConfigManager::~ConfigManager() {}

    bool ConfigManager::LoadFromFile(const std::string& filepath) {
        ASTRAL_LOG_WARN("ConfigManager::LoadFromFile henüz implemente edilmedi.");
        return false;
    }

    std::string ConfigManager::GetString(const std::string& key, const std::string& defaultValue) {
        return defaultValue;
    }

    int ConfigManager::GetInt(const std::string& key, int defaultValue) {
        return defaultValue;
    }
}
