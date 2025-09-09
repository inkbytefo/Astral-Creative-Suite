#include "ConfigManager.h"
#include "Logger.h"

namespace AstralEngine {
    ConfigManager::ConfigManager() {
        AE_INFO("ConfigManager oluşturuldu.");
    }

    ConfigManager::~ConfigManager() {}

    bool ConfigManager::LoadFromFile(const std::string& filepath) {
        AE_WARN("ConfigManager::LoadFromFile henüz implemente edilmedi.");
        return false;
    }

    std::string ConfigManager::GetString(const std::string& key, const std::string& defaultValue) {
        return defaultValue;
    }

    int ConfigManager::GetInt(const std::string& key, int defaultValue) {
        return defaultValue;
    }
}
