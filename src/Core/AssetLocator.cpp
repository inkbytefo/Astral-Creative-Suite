#include "AssetLocator.h"
#include "Logger.h"
#include <filesystem>
#include <algorithm>
#include <fstream>
#include <sstream>

namespace AstralEngine {
    void AssetLocator::initialize(const std::string& executablePath) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_executablePath = executablePath;
        
        // Determine base asset path
        std::filesystem::path exePath(executablePath);
        std::filesystem::path parentPath = exePath.parent_path();
        
        // Try common asset directory locations
        std::vector<std::string> possibleAssetPaths = {
            (parentPath / "assets").string(),
            (parentPath / ".." / "assets").string(),
            (parentPath / ".." / ".." / "assets").string(),
            "assets"
        };
        
        for (const auto& path : possibleAssetPaths) {
            if (std::filesystem::exists(path)) {
                m_baseAssetPath = path;
                break;
            }
        }
        
        // Add default search paths
        if (!m_baseAssetPath.empty()) {
            m_searchPaths.push_back(m_baseAssetPath);
            m_searchPaths.push_back(m_baseAssetPath + "/textures");
            m_searchPaths.push_back(m_baseAssetPath + "/shaders");
            m_searchPaths.push_back(m_baseAssetPath + "/models");
        }
        
        std::stringstream ss;
        ss << "AssetLocator initialized with base path: " << m_baseAssetPath;
        AE_INFO(ss.str());
    }
    
    bool AssetLocator::validateCriticalAssets() {
        std::lock_guard<std::mutex> lock(m_mutex);
        // For now, we'll just check if the base asset path exists
        if (m_baseAssetPath.empty()) {
            AE_ERROR("Base asset path is not set");
            return false;
        }
        
        if (!std::filesystem::exists(m_baseAssetPath)) {
            std::stringstream ss;
            ss << "Base asset path does not exist: " << m_baseAssetPath;
            AE_ERROR(ss.str());
            return false;
        }
        
        AE_INFO("Asset path validation successful");
        return true;
    }
    
    std::string AssetLocator::getAssetPath(const std::string& assetName) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        // First check if it's already a full path
        if (fileExists(assetName)) {
            return assetName;
        }
        
        // Check in search paths
        for (const auto& path : m_searchPaths) {
            std::filesystem::path fullPath = std::filesystem::path(path) / assetName;
            if (fileExists(fullPath.string())) {
                return fullPath.string();
            }
        }
        
        // If not found in search paths, try relative to base asset path
        if (!m_baseAssetPath.empty()) {
            std::filesystem::path fullPath = std::filesystem::path(m_baseAssetPath) / assetName;
            if (fileExists(fullPath.string())) {
                return fullPath.string();
            }
        }
        
        // If still not found, return the original name
        std::stringstream ss;
        ss << "Asset not found in any search path: " << assetName;
        AE_WARN(ss.str());
        return assetName;
    }
    
    std::string AssetLocator::getPlatformSpecificPath(const std::string& assetName) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        // For now, we'll just return the normal asset path
        // In the future, this could handle platform-specific assets
        return getAssetPath(assetName);
    }
    
    void AssetLocator::addSearchPath(const std::string& path) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (std::find(m_searchPaths.begin(), m_searchPaths.end(), path) == m_searchPaths.end()) {
            m_searchPaths.push_back(path);
            std::stringstream ss;
            ss << "Added search path: " << path;
            AE_INFO(ss.str());
        }
    }
    
    void AssetLocator::removeSearchPath(const std::string& path) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = std::find(m_searchPaths.begin(), m_searchPaths.end(), path);
        if (it != m_searchPaths.end()) {
            m_searchPaths.erase(it);
            std::stringstream ss;
            ss << "Removed search path: " << path;
            AE_INFO(ss.str());
        }
    }
    
    const std::vector<std::string>& AssetLocator::getSearchPaths() const {
        return m_searchPaths;
    }
    
    bool AssetLocator::fileExists(const std::string& filePath) const {
        return std::filesystem::exists(filePath);
    }
    
    std::string AssetLocator::getExecutablePath() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_executablePath;
    }
    
    std::string AssetLocator::getBaseAssetPath() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_baseAssetPath;
    }
}
