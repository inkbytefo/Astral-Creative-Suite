#ifndef ASTRAL_ENGINE_ASSET_DEPENDENCY_H
#define ASTRAL_ENGINE_ASSET_DEPENDENCY_H

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <mutex>

namespace AstralEngine {
    class AssetDependency {
    public:
        AssetDependency(const std::string& assetPath);
        
        const std::string& getAssetPath() const;
        void addDependency(const std::string& dependency);
        void removeDependency(const std::string& dependency);
        const std::vector<std::string>& getDependencies() const;
        bool hasDependency(const std::string& dependency) const;
        
        // Static methods for global dependency management
        static void addAssetDependency(const std::string& assetPath, const std::string& dependency);
        static void removeAssetDependency(const std::string& assetPath, const std::string& dependency);
        static std::vector<std::string> getAssetDependencies(const std::string& assetPath);
        static bool hasAssetDependency(const std::string& assetPath, const std::string& dependency);
        static std::vector<std::string> resolveDependencyOrder(const std::vector<std::string>& assets);
        static bool hasCircularDependency(const std::string& assetPath);
        
    private:
        std::string m_assetPath;
        std::vector<std::string> m_dependencies;
        mutable std::mutex m_mutex;
        
        // Static members for global dependency tracking
        static std::unordered_map<std::string, std::shared_ptr<AssetDependency>> s_assetDependencies;
        static std::mutex s_globalMutex;
        
        // Helper methods
        static bool hasCircularDependencyHelper(const std::string& assetPath, 
                                              std::unordered_set<std::string>& visited, 
                                              std::unordered_set<std::string>& recursionStack);
    };
}

#endif // ASTRAL_ENGINE_ASSET_DEPENDENCY_H
