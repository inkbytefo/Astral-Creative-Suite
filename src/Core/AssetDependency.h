#pragma once

#include <string>
#include <vector>

namespace AstralEngine {
    class AssetDependency {
    public:
        AssetDependency(const std::string& assetPath) : m_assetPath(assetPath) {}
        
        const std::string& getAssetPath() const { return m_assetPath; }
        void addDependency(const std::string& dependency) { m_dependencies.push_back(dependency); }
        const std::vector<std::string>& getDependencies() const { return m_dependencies; }
        
    private:
        std::string m_assetPath;
        std::vector<std::string> m_dependencies;
    };
}