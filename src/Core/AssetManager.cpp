#include "AssetManager.h"
#include "Logger.h"
#include "Asset/ModelAsset.h"
#include <algorithm>

namespace AstralEngine {
    // Static member definitions
    std::unordered_map<std::string, std::shared_ptr<void>> AssetManager::s_assets;
    std::mutex AssetManager::s_mutex;
    
    void AssetManager::init() {
        std::lock_guard<std::mutex> lock(s_mutex);
        // Initialize asset manager
        AE_INFO("AssetManager initialized");
    }
    
    void AssetManager::shutdown() {
        std::lock_guard<std::mutex> lock(s_mutex);
        // Shutdown asset manager
        s_assets.clear();
        AE_INFO("AssetManager shutdown");
    }
    
    bool AssetManager::isAssetLoaded(const std::string& assetPath) {
        std::lock_guard<std::mutex> lock(s_mutex);
        return s_assets.find(assetPath) != s_assets.end();
    }
    
    void AssetManager::unloadAllAssets() {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_assets.clear();
        AE_INFO("All assets unloaded");
    }

    // Template specialization for ModelAsset
    template<>
    std::shared_ptr<ModelAsset> AssetManager::loadAsset<ModelAsset>(const std::string& assetPath) {
        std::lock_guard<std::mutex> lock(s_mutex);
        
        // Check if asset is already loaded
        auto it = s_assets.find(assetPath);
        if (it != s_assets.end()) {
            return std::static_pointer_cast<ModelAsset>(it->second);
        }
        
        // Asset not loaded, so create and load it
        AE_DEBUG("Creating and loading new ModelAsset: {}", assetPath);
        auto asset = std::make_shared<ModelAsset>(assetPath);
        asset->load(); // Synchronous load for now

        if (asset->isLoaded()) {
            s_assets[assetPath] = asset;
            return asset;
        }

        AE_ERROR("Failed to load ModelAsset: {}", assetPath);
        return nullptr;
    }
}
