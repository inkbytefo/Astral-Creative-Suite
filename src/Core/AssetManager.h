#ifndef ASTRAL_ENGINE_ASSET_MANAGER_H
#define ASTRAL_ENGINE_ASSET_MANAGER_H

#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include "Core/Logger.h" // For logging

namespace AstralEngine {
    // Forward declare Asset types
    class ModelAsset;

    class AssetManager {
    public:
        static void init();
        static void shutdown();
        
        template<typename T>
        static std::shared_ptr<T> loadAsset(const std::string& assetPath);
        
        template<typename T>
        static void unloadAsset(const std::string& assetPath);
        
        template<typename T>
        static std::shared_ptr<T> getAsset(const std::string& assetPath);
        
        static bool isAssetLoaded(const std::string& assetPath);
        static void unloadAllAssets();
        
    private:
        static std::unordered_map<std::string, std::shared_ptr<void>> s_assets;
        static std::mutex s_mutex;
    };
    
    // Generic template implementation for assets that don't have a specialization
    template<typename T>
    std::shared_ptr<T> AssetManager::loadAsset(const std::string& assetPath) {
        AE_WARN("loadAsset not implemented for this asset type.");
        return nullptr;
    }
    
    // Template specialization for ModelAsset
    template<>
    std::shared_ptr<ModelAsset> AssetManager::loadAsset<ModelAsset>(const std::string& assetPath);


    template<typename T>
    void AssetManager::unloadAsset(const std::string& assetPath) {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_assets.erase(assetPath);
    }
    
    template<typename T>
    std::shared_ptr<T> AssetManager::getAsset(const std::string& assetPath) {
        std::lock_guard<std::mutex> lock(s_mutex);
        auto it = s_assets.find(assetPath);
        if (it != s_assets.end()) {
            return std::static_pointer_cast<T>(it->second);
        }
        return nullptr;
    }
}

#endif // ASTRAL_ENGINE_ASSET_MANAGER_H
