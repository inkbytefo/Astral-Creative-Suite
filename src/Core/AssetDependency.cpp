#include "AssetDependency.h"
#include "AssetManager.h"
#include "Logger.h"
#include "Renderer/UnifiedMaterial.h"
#include "Renderer/Texture.h"
#include "Renderer/Vulkan/VulkanDevice.h"

#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <thread>
#include <glm/glm.hpp>

namespace AstralEngine {

    // MaterialLibrary implementation
    bool MaterialLibrary::parseMTL(const std::string& mtlPath, 
                                 std::vector<MaterialInfo>& materials,
                                 std::string& error) {
        std::ifstream file(mtlPath);
        if (!file.is_open()) {
            error = "Failed to open MTL file: " + mtlPath;
            return false;
        }

        MaterialInfo currentMaterial;
        bool hasMaterial = false;
        std::string line;

        AE_DEBUG("Parsing MTL file: {}", mtlPath);

        while (std::getline(file, line)) {
            // Trim whitespace
            line.erase(0, line.find_first_not_of(" \t"));
            line.erase(line.find_last_not_of(" \t") + 1);

            // Skip empty lines and comments
            if (line.empty() || line[0] == '#') {
                continue;
            }

            // New material definition
            if (line.substr(0, 7) == "newmtl ") {
                // Save previous material if exists
                if (hasMaterial && !currentMaterial.name.empty()) {
                    materials.push_back(currentMaterial);
                }
                
                // Start new material
                currentMaterial = MaterialInfo();
                currentMaterial.name = line.substr(7);
                hasMaterial = true;
                continue;
            }

            // Parse material properties only if we have an active material
            if (hasMaterial) {
                parseMTLLine(line, currentMaterial);
            }
        }

        // Add final material
        if (hasMaterial && !currentMaterial.name.empty()) {
            materials.push_back(currentMaterial);
        }

        AE_INFO("Loaded {} materials from MTL file: {}", materials.size(), mtlPath);
        return true;
    }

    void MaterialLibrary::parseMTLLine(const std::string& line, MaterialInfo& material) {
        std::istringstream iss(line);
        std::string token;
        iss >> token;

        if (token == "Kd") {
            // Diffuse color
            iss >> material.baseColor.r >> material.baseColor.g >> material.baseColor.b;
        }
        else if (token == "Ke") {
            // Emissive color
            iss >> material.emissive.r >> material.emissive.g >> material.emissive.b;
        }
        else if (token == "Ns") {
            // Specular exponent -> convert to roughness
            float ns;
            iss >> ns;
            material.roughness = std::max(0.01f, 1.0f - (ns / 1000.0f));
        }
        else if (token == "Ni") {
            // Index of refraction (not used in simple PBR)
        }
        else if (token == "d" || token == "Tr") {
            // Transparency
            float alpha;
            iss >> alpha;
            if (token == "Tr") alpha = 1.0f - alpha; // Tr is inverted
            material.isTransparent = (alpha < 1.0f);
            material.alphaCutoff = alpha;
        }
        else if (token == "map_Kd") {
            // Diffuse texture map
            std::string remaining;
            std::getline(iss, remaining);
            material.diffuseTexture = remaining.substr(remaining.find_first_not_of(" \t"));
        }
        else if (token == "map_Bump" || token == "bump" || token == "map_Kn") {
            // Normal map
            std::string remaining;
            std::getline(iss, remaining);
            material.normalTexture = remaining.substr(remaining.find_first_not_of(" \t"));
        }
        else if (token == "map_Pm" || token == "map_Pr") {
            // Metallic map or metallic-roughness combined
            std::string remaining;
            std::getline(iss, remaining);
            material.metallicRoughnessTexture = remaining.substr(remaining.find_first_not_of(" \t"));
        }
        else if (token == "map_Ka") {
            // Ambient occlusion map
            std::string remaining;
            std::getline(iss, remaining);
            material.occlusionTexture = remaining.substr(remaining.find_first_not_of(" \t"));
        }
        else if (token == "map_Ke") {
            // Emissive map
            std::string remaining;
            std::getline(iss, remaining);
            material.emissiveTexture = remaining.substr(remaining.find_first_not_of(" \t"));
        }
        // Add other material properties as needed
    }

    std::shared_ptr<UnifiedMaterialInstance> MaterialLibrary::createMaterial(
        const MaterialInfo& info,
        const std::string& baseDirectory,
        const std::unordered_map<std::string, std::shared_ptr<Texture>>& loadedTextures) {
        
        auto material = std::make_shared<UnifiedMaterialInstance>();
        *material = UnifiedMaterialInstance::createDefault();

        // Set base properties
        material->setBaseColor(glm::vec4(info.baseColor, info.isTransparent ? info.alphaCutoff : 1.0f));
        material->setMetallic(info.metallic);
        material->setRoughness(info.roughness);
        material->setEmissive(info.emissive);

        // Set transparency properties
        if (info.isTransparent) {
            material->setAlphaMode(AlphaMode::Blend);
            material->setAlphaTest(info.alphaCutoff);
        }

        if (info.isDoubleSided) {
            material->setRenderingFlag(RenderingFlags::DoubleSided, true);
        }

        // Bind textures if available
        auto bindTexture = [&](const std::string& texPath, int slot) {
            if (!texPath.empty()) {
                std::string fullPath = baseDirectory + "/" + texPath;
                auto it = loadedTextures.find(fullPath);
                if (it != loadedTextures.end()) {
                    material->setTexture(static_cast<TextureSlot>(slot), it->second);
                }
            }
        };

        bindTexture(info.diffuseTexture, static_cast<int>(TextureSlot::BaseColor));
        bindTexture(info.normalTexture, static_cast<int>(TextureSlot::Normal));
        bindTexture(info.metallicRoughnessTexture, static_cast<int>(TextureSlot::MetallicRoughness));
        bindTexture(info.occlusionTexture, static_cast<int>(TextureSlot::Occlusion));
        bindTexture(info.emissiveTexture, static_cast<int>(TextureSlot::Emissive));

        AE_DEBUG("Created material '{}' from MTL", info.name);
        return material;
    }

    std::vector<std::string> MaterialLibrary::extractTexturePaths(
        const MaterialInfo& info, 
        const std::string& baseDirectory) {
        
        std::vector<std::string> paths;
        
        auto addPath = [&](const std::string& texPath) {
            if (!texPath.empty()) {
                paths.push_back(baseDirectory + "/" + texPath);
            }
        };

        addPath(info.diffuseTexture);
        addPath(info.normalTexture);
        addPath(info.metallicRoughnessTexture);
        addPath(info.occlusionTexture);
        addPath(info.emissiveTexture);

        return paths;
    }

    // DependencyGraph implementation
    void DependencyGraph::addDependency(std::shared_ptr<AssetDependency> dependency) {
        m_dependencies.push_back(dependency);
        m_dependencyMap[dependency->path] = dependency;
    }

    void DependencyGraph::loadDependencies(Vulkan::VulkanDevice& device, LoadCallback callback) {
        if (m_dependencies.empty()) {
            callback(true, "");
            return;
        }

        AE_INFO("Loading {} asset dependencies", m_dependencies.size());

        // Load all texture dependencies asynchronously
        for (auto& dep : m_dependencies) {
            if (dep->type == AssetDependency::Type::TEXTURE) {
                loadTextureDependency(dep, device);
            }
        }

        // Start a monitoring thread to check completion
        std::thread([this, callback]() {
            checkLoadingComplete(callback);
        }).detach();
    }

    void DependencyGraph::loadTextureDependency(std::shared_ptr<AssetDependency> dep, Vulkan::VulkanDevice& device) {
        dep->state = AssetDependency::State::LOADING;

        try {
            auto future = AssetManager::loadTextureAsync(device, dep->path);
            
            // Monitor the future in a separate thread
            std::thread([dep, future = std::move(future)]() mutable {
                try {
                    auto texture = future.get();
                    dep->asset = texture;
                    dep->state = AssetDependency::State::LOADED;
                    AE_DEBUG("Loaded texture dependency: {}", dep->path);
                } catch (const std::exception& e) {
                    dep->errorMessage = e.what();
                    dep->state = AssetDependency::State::FAILED;
                    AE_WARN("Failed to load texture dependency {}: {}", dep->path, e.what());
                }
            }).detach();

        } catch (const std::exception& e) {
            dep->errorMessage = e.what();
            dep->state = AssetDependency::State::FAILED;
            AE_ERROR("Failed to start texture loading for {}: {}", dep->path, e.what());
        }
    }

    void DependencyGraph::checkLoadingComplete(LoadCallback callback) {
        const int maxWaitTimeMs = 30000; // 30 seconds timeout
        const int checkIntervalMs = 100;
        int waitedMs = 0;

        while (waitedMs < maxWaitTimeMs) {
            bool allComplete = true;
            bool anyFailed = false;
            std::string errorMsg;

            for (const auto& dep : m_dependencies) {
                if (dep->state == AssetDependency::State::LOADING) {
                    allComplete = false;
                    break;
                }
                if (dep->state == AssetDependency::State::FAILED) {
                    anyFailed = true;
                    if (errorMsg.empty()) {
                        errorMsg = "Failed to load: " + dep->path + " (" + dep->errorMessage + ")";
                    }
                }
            }

            if (allComplete) {
                if (anyFailed) {
                    callback(false, errorMsg);
                } else {
                    AE_INFO("All dependencies loaded successfully");
                    callback(true, "");
                }
                return;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(checkIntervalMs));
            waitedMs += checkIntervalMs;
        }

        // Timeout
        callback(false, "Dependency loading timeout");
    }

    bool DependencyGraph::isFullyLoaded() const {
        for (const auto& dep : m_dependencies) {
            if (dep->state != AssetDependency::State::LOADED) {
                return false;
            }
        }
        return true;
    }

    std::shared_ptr<void> DependencyGraph::getAsset(const std::string& path) const {
        auto it = m_dependencyMap.find(path);
        if (it != m_dependencyMap.end() && it->second->state == AssetDependency::State::LOADED) {
            return it->second->asset;
        }
        return nullptr;
    }

    std::shared_ptr<Texture> DependencyGraph::getTexture(const std::string& path) const {
        auto asset = getAsset(path);
        return std::static_pointer_cast<Texture>(asset);
    }

    float DependencyGraph::getLoadingProgress() const {
        if (m_dependencies.empty()) {
            return 1.0f;
        }

        size_t completedCount = 0;
        for (const auto& dep : m_dependencies) {
            if (dep->state == AssetDependency::State::LOADED || 
                dep->state == AssetDependency::State::FAILED) {
                completedCount++;
            }
        }

        return static_cast<float>(completedCount) / static_cast<float>(m_dependencies.size());
    }

    std::shared_ptr<UnifiedMaterialInstance> DependencyGraph::getMaterial(const std::string& name) const {
        // For materials we search by name in the path field
        for (const auto& dep : m_dependencies) {
            if (dep->type == AssetDependency::Type::MATERIAL && 
                dep->path == name && 
                dep->state == AssetDependency::State::LOADED) {
                return std::static_pointer_cast<UnifiedMaterialInstance>(dep->asset);
            }
        }
        return nullptr;
    }

} // namespace AstralEngine
