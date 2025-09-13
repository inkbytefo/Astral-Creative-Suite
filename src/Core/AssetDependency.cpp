#include "AssetDependency.h"
#include "Logger.h"
#include <algorithm>
#include <queue>
#include <sstream>
#include <functional>

namespace AstralEngine {
    // Static member definitions
    std::unordered_map<std::string, std::shared_ptr<AssetDependency>> AssetDependency::s_assetDependencies;
    std::mutex AssetDependency::s_globalMutex;
    
    AssetDependency::AssetDependency(const std::string& assetPath) : m_assetPath(assetPath) {}
    
    const std::string& AssetDependency::getAssetPath() const {
        return m_assetPath;
    }
    
    void AssetDependency::addDependency(const std::string& dependency) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (std::find(m_dependencies.begin(), m_dependencies.end(), dependency) == m_dependencies.end()) {
            m_dependencies.push_back(dependency);
        }
    }
    
    void AssetDependency::removeDependency(const std::string& dependency) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = std::find(m_dependencies.begin(), m_dependencies.end(), dependency);
        if (it != m_dependencies.end()) {
            m_dependencies.erase(it);
        }
    }
    
    const std::vector<std::string>& AssetDependency::getDependencies() const {
        return m_dependencies;
    }
    
    bool AssetDependency::hasDependency(const std::string& dependency) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return std::find(m_dependencies.begin(), m_dependencies.end(), dependency) != m_dependencies.end();
    }
    
    // Static methods
    void AssetDependency::addAssetDependency(const std::string& assetPath, const std::string& dependency) {
        std::lock_guard<std::mutex> lock(s_globalMutex);
        auto it = s_assetDependencies.find(assetPath);
        if (it == s_assetDependencies.end()) {
            auto assetDep = std::make_shared<AssetDependency>(assetPath);
            assetDep->addDependency(dependency);
            s_assetDependencies[assetPath] = assetDep;
        } else {
            it->second->addDependency(dependency);
        }
    }
    
    void AssetDependency::removeAssetDependency(const std::string& assetPath, const std::string& dependency) {
        std::lock_guard<std::mutex> lock(s_globalMutex);
        auto it = s_assetDependencies.find(assetPath);
        if (it != s_assetDependencies.end()) {
            it->second->removeDependency(dependency);
        }
    }
    
    std::vector<std::string> AssetDependency::getAssetDependencies(const std::string& assetPath) {
        std::lock_guard<std::mutex> lock(s_globalMutex);
        auto it = s_assetDependencies.find(assetPath);
        if (it != s_assetDependencies.end()) {
            return it->second->getDependencies();
        }
        return {};
    }
    
    bool AssetDependency::hasAssetDependency(const std::string& assetPath, const std::string& dependency) {
        std::lock_guard<std::mutex> lock(s_globalMutex);
        auto it = s_assetDependencies.find(assetPath);
        if (it != s_assetDependencies.end()) {
            return it->second->hasDependency(dependency);
        }
        return false;
    }
    
    std::vector<std::string> AssetDependency::resolveDependencyOrder(const std::vector<std::string>& assets) {
        std::lock_guard<std::mutex> lock(s_globalMutex);
        std::vector<std::string> result;
        std::unordered_set<std::string> visited;
        std::unordered_set<std::string> tempVisited;
        
        // Build dependency graph
        std::unordered_map<std::string, std::vector<std::string>> graph;
        for (const auto& asset : assets) {
            auto it = s_assetDependencies.find(asset);
            if (it != s_assetDependencies.end()) {
                graph[asset] = it->second->getDependencies();
            } else {
                graph[asset] = {};
            }
        }
        
        // Topological sort using DFS
        std::function<bool(const std::string&)> visit = [&](const std::string& asset) -> bool {
            if (tempVisited.find(asset) != tempVisited.end()) {
                // Circular dependency detected
                std::string errorMsg = "Circular dependency detected involving asset: " + asset;
                AE_ERROR(errorMsg);
                return false;
            }
            
            if (visited.find(asset) == visited.end()) {
                tempVisited.insert(asset);
                
                auto it = graph.find(asset);
                if (it != graph.end()) {
                    for (const auto& dep : it->second) {
                        if (!visit(dep)) {
                            return false;
                        }
                    }
                }
                
                tempVisited.erase(asset);
                visited.insert(asset);
                result.push_back(asset);
            }
            
            return true;
        };
        
        // Visit all assets
        for (const auto& asset : assets) {
            if (visited.find(asset) == visited.end()) {
                if (!visit(asset)) {
                    // Circular dependency detected, return empty vector
                    return {};
                }
            }
        }
        
        return result;
    }
    
    bool AssetDependency::hasCircularDependency(const std::string& assetPath) {
        std::lock_guard<std::mutex> lock(s_globalMutex);
        std::unordered_set<std::string> visited;
        std::unordered_set<std::string> recursionStack;
        return hasCircularDependencyHelper(assetPath, visited, recursionStack);
    }
    
    bool AssetDependency::hasCircularDependencyHelper(const std::string& assetPath, 
                                                   std::unordered_set<std::string>& visited, 
                                                   std::unordered_set<std::string>& recursionStack) {
        if (recursionStack.find(assetPath) != recursionStack.end()) {
            return true;
        }
        
        if (visited.find(assetPath) != visited.end()) {
            return false;
        }
        
        visited.insert(assetPath);
        recursionStack.insert(assetPath);
        
        auto it = s_assetDependencies.find(assetPath);
        if (it != s_assetDependencies.end()) {
            for (const auto& dep : it->second->getDependencies()) {
                if (hasCircularDependencyHelper(dep, visited, recursionStack)) {
                    return true;
                }
            }
        }
        
        recursionStack.erase(assetPath);
        return false;
    }
}
