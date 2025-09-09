#include "AssetLocator.h"
#include "Logger.h"
#include <filesystem>
#include <fstream>

namespace AstralEngine {

AssetLocator& AssetLocator::getInstance() {
    static AssetLocator instance;
    return instance;
}

void AssetLocator::initialize(const std::string& executablePath) {
    AE_DEBUG("Initializing AssetLocator with executable: %s", executablePath.c_str());
    
    detectDirectories(executablePath);
    
    // Add default search paths for shaders
    m_shaderSearchPaths.clear();
    m_shaderSearchPaths.push_back(m_shaderDir);
    m_shaderSearchPaths.push_back(m_executableDir + "/shaders");
    m_shaderSearchPaths.push_back(m_executableDir + "/Debug/shaders");
    m_shaderSearchPaths.push_back(m_executableDir + "/Release/shaders");
    m_shaderSearchPaths.push_back(m_executableDir + "/../shaders");
    m_shaderSearchPaths.push_back("./shaders");
    m_shaderSearchPaths.push_back("../shaders");
    
    // Add default search paths for assets
    m_assetSearchPaths.clear();
    m_assetSearchPaths.push_back(m_assetDir);
    m_assetSearchPaths.push_back(m_executableDir + "/assets");
    m_assetSearchPaths.push_back(m_executableDir + "/Debug/assets");
    m_assetSearchPaths.push_back(m_executableDir + "/Release/assets");
    m_assetSearchPaths.push_back(m_executableDir + "/../assets");
    m_assetSearchPaths.push_back("./assets");
    m_assetSearchPaths.push_back("../assets");
    
    AE_INFO("AssetLocator initialized:");
    AE_INFO("  Executable dir: %s", m_executableDir.c_str());
    AE_INFO("  Shader dir: %s", m_shaderDir.c_str());
    AE_INFO("  Asset dir: %s", m_assetDir.c_str());
}

void AssetLocator::detectDirectories(const std::string& executablePath) {
    try {
        // Get executable directory
        std::filesystem::path exePath(executablePath);
        m_executableDir = exePath.parent_path().string();
        
        // Try to find shader directory
        std::vector<std::string> shaderCandidates = {
            m_executableDir + "/shaders",
            m_executableDir + "/Debug/shaders",
            m_executableDir + "/Release/shaders",
            m_executableDir + "/../shaders"
        };
        
        for (const auto& candidate : shaderCandidates) {
            if (std::filesystem::exists(candidate)) {
                m_shaderDir = std::filesystem::canonical(candidate).string();
                break;
            }
        }
        
        // Default fallback
        if (m_shaderDir.empty()) {
            m_shaderDir = m_executableDir + "/shaders";
        }
        
        // Try to find asset directory
        std::vector<std::string> assetCandidates = {
            m_executableDir + "/assets",
            m_executableDir + "/Debug/assets",
            m_executableDir + "/Release/assets",
            m_executableDir + "/../assets"
        };
        
        for (const auto& candidate : assetCandidates) {
            if (std::filesystem::exists(candidate)) {
                m_assetDir = std::filesystem::canonical(candidate).string();
                break;
            }
        }
        
        // Default fallback
        if (m_assetDir.empty()) {
            m_assetDir = m_executableDir + "/assets";
        }
        
    } catch (const std::exception& e) {
        AE_ERROR("Failed to detect asset directories: %s", e.what());
        // Use safe defaults
        m_executableDir = ".";
        m_shaderDir = "./shaders";
        m_assetDir = "./assets";
    }
}

std::string AssetLocator::resolveShaderPath(const std::string& relativePath) const {
    // Try direct path first
    if (std::filesystem::exists(relativePath)) {
        return std::filesystem::absolute(relativePath).string();
    }
    
    // Search in shader search paths
    std::string resolved = findFileInPaths(relativePath, m_shaderSearchPaths);
    if (!resolved.empty()) {
        AE_DEBUG("Resolved shader: %s -> %s", relativePath.c_str(), resolved.c_str());
        return resolved;
    }
    
    // Log detailed search attempt
    AE_DEBUG("Shader resolution failed for: %s", relativePath.c_str());
    AE_DEBUG("Searched in:");
    for (const auto& path : m_shaderSearchPaths) {
        std::string fullPath = path + "/" + relativePath;
        bool exists = std::filesystem::exists(fullPath);
        AE_DEBUG("  %s [%s]", fullPath.c_str(), exists ? "EXISTS" : "NOT FOUND");
    }
    
    // Return original path as fallback
    return relativePath;
}

std::string AssetLocator::resolveAssetPath(const std::string& relativePath) const {
    // Try direct path first
    if (std::filesystem::exists(relativePath)) {
        return std::filesystem::absolute(relativePath).string();
    }
    
    // Search in asset search paths
    std::string resolved = findFileInPaths(relativePath, m_assetSearchPaths);
    if (!resolved.empty()) {
        AE_DEBUG("Resolved asset: %s -> %s", relativePath.c_str(), resolved.c_str());
        return resolved;
    }
    
    // Return original path as fallback
    return relativePath;
}

std::string AssetLocator::findFileInPaths(const std::string& filename, const std::vector<std::string>& paths) const {
    for (const auto& basePath : paths) {
        std::string fullPath = basePath + "/" + filename;
        if (std::filesystem::exists(fullPath)) {
            try {
                return std::filesystem::canonical(fullPath).string();
            } catch (const std::exception&) {
                return std::filesystem::absolute(fullPath).string();
            }
        }
    }
    return "";
}

bool AssetLocator::validateCriticalAssets() const {
    AE_INFO("Validating critical engine assets...");
    
    bool allFound = true;
    
    for (const auto& shader : m_criticalShaders) {
        std::string path = resolveShaderPath(shader);
        bool exists = std::filesystem::exists(path);
        
        if (exists) {
            AE_DEBUG("  ✓ %s -> %s", shader.c_str(), path.c_str());
        } else {
            AE_ERROR("  ✗ %s (NOT FOUND)", shader.c_str());
            allFound = false;
        }
    }
    
    if (allFound) {
        AE_INFO("All critical assets validated successfully");
    } else {
        AE_ERROR("Critical asset validation failed - some required files are missing");
        AE_ERROR("Please run shader compilation: compile_shaders.bat");
    }
    
    return allFound;
}

void AssetLocator::addShaderSearchPath(const std::string& path) {
    m_shaderSearchPaths.insert(m_shaderSearchPaths.begin(), path);
    AE_DEBUG("Added shader search path: %s", path.c_str());
}

void AssetLocator::addAssetSearchPath(const std::string& path) {
    m_assetSearchPaths.insert(m_assetSearchPaths.begin(), path);
    AE_DEBUG("Added asset search path: %s", path.c_str());
}

void AssetLocator::logSearchPaths() const {
    AE_DEBUG("=== AssetLocator Search Paths ===");
    AE_DEBUG("Shader search paths:");
    for (size_t i = 0; i < m_shaderSearchPaths.size(); ++i) {
        bool exists = std::filesystem::exists(m_shaderSearchPaths[i]);
        AE_DEBUG("  %zu. %s [%s]", i + 1, m_shaderSearchPaths[i].c_str(), 
                exists ? "EXISTS" : "MISSING");
    }
    
    AE_DEBUG("Asset search paths:");
    for (size_t i = 0; i < m_assetSearchPaths.size(); ++i) {
        bool exists = std::filesystem::exists(m_assetSearchPaths[i]);
        AE_DEBUG("  %zu. %s [%s]", i + 1, m_assetSearchPaths[i].c_str(), 
                exists ? "EXISTS" : "MISSING");
    }
    AE_DEBUG("================================");
}

} // namespace AstralEngine
