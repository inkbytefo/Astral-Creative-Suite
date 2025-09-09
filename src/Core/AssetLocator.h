#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace AstralEngine {

/**
 * @brief Central asset and shader path resolution system for the engine
 * 
 * Provides deterministic path resolution that works across different working directories
 * and build configurations (Debug/Release). Handles shader compilation and caching.
 */
class AssetLocator {
public:
    static AssetLocator& getInstance();
    
    // Initialize asset locator with executable path
    void initialize(const std::string& executablePath);
    
    // Shader resolution
    std::string resolveShaderPath(const std::string& relativePath) const;
    std::string resolveAssetPath(const std::string& relativePath) const;
    
    // Verify critical assets exist at startup
    bool validateCriticalAssets() const;
    
    // Get various base directories
    std::string getShaderDirectory() const { return m_shaderDir; }
    std::string getAssetDirectory() const { return m_assetDir; }
    std::string getExecutableDirectory() const { return m_executableDir; }
    
    // Add custom search paths
    void addShaderSearchPath(const std::string& path);
    void addAssetSearchPath(const std::string& path);
    
    // Debug information
    void logSearchPaths() const;

private:
    AssetLocator() = default;
    
    void detectDirectories(const std::string& executablePath);
    std::string findFileInPaths(const std::string& filename, const std::vector<std::string>& paths) const;
    
    std::string m_executableDir;
    std::string m_shaderDir;
    std::string m_assetDir;
    
    std::vector<std::string> m_shaderSearchPaths;
    std::vector<std::string> m_assetSearchPaths;
    
    // Critical assets that must exist
    std::vector<std::string> m_criticalShaders = {
        "basic.vert.spv",
        "unified_pbr.frag.spv",
        "unified_pbr.vert.spv",
        "shadow_depth.vert.spv"
    };
};

} // namespace AstralEngine
