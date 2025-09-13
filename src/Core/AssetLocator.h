#ifndef ASTRAL_ENGINE_ASSET_LOCATOR_H
#define ASTRAL_ENGINE_ASSET_LOCATOR_H

#include <string>
#include <vector>
#include <mutex>

namespace AstralEngine {
    class AssetLocator {
    public:
        static AssetLocator& getInstance() {
            static AssetLocator instance;
            return instance;
        }
        
        void initialize(const std::string& executablePath);
        bool validateCriticalAssets();
        std::string getAssetPath(const std::string& assetName) const;
        
        // Platform-specific path handling
        std::string getPlatformSpecificPath(const std::string& assetName) const;
        
        // Asset search paths
        void addSearchPath(const std::string& path);
        void removeSearchPath(const std::string& path);
        const std::vector<std::string>& getSearchPaths() const;
        
        // Utility functions
        bool fileExists(const std::string& filePath) const;
        std::string getExecutablePath() const;
        std::string getBaseAssetPath() const;
        
    private:
        AssetLocator() = default;
        ~AssetLocator() = default;
        
        std::string m_executablePath;
        std::string m_baseAssetPath;
        std::vector<std::string> m_searchPaths;
        mutable std::mutex m_mutex;
    };
}

#endif // ASTRAL_ENGINE_ASSET_LOCATOR_H
