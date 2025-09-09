#pragma once

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <future>
#include <glm/glm.hpp>

namespace AstralEngine {

    // Forward declarations
    class UnifiedMaterialInstance;
    class Texture;
    namespace Vulkan {
        class VulkanDevice;
    }

    /**
     * @brief Represents an asset dependency with loading state
     */
    struct AssetDependency {
        enum class Type {
            TEXTURE,
            MATERIAL,
            MODEL
        };

        enum class State {
            UNLOADED,
            LOADING,
            LOADED,
            FAILED
        };

        std::string path;
        Type type;
        State state = State::UNLOADED;
        std::shared_ptr<void> asset; // Generic asset pointer
        std::string errorMessage;

        AssetDependency(const std::string& assetPath, Type assetType)
            : path(assetPath), type(assetType) {}
    };

    /**
     * @brief Manages material library (.mtl) parsing and material creation
     */
    class MaterialLibrary {
    public:
        struct MaterialInfo {
            std::string name;
            std::string diffuseTexture;
            std::string normalTexture;
            std::string metallicRoughnessTexture;
            std::string occlusionTexture;
            std::string emissiveTexture;
            
            // PBR properties
            glm::vec3 baseColor = {1.0f, 1.0f, 1.0f};
            float metallic = 0.0f;
            float roughness = 1.0f;
            glm::vec3 emissive = {0.0f, 0.0f, 0.0f};
            float alphaCutoff = 0.5f;
            bool isTransparent = false;
            bool isDoubleSided = false;
        };

        /**
         * @brief Parse MTL file and extract material information
         */
        static bool parseMTL(const std::string& mtlPath, 
                            std::vector<MaterialInfo>& materials,
                            std::string& error);

        /**
         * @brief Create UnifiedMaterialInstance from MaterialInfo
         */
        static std::shared_ptr<UnifiedMaterialInstance> createMaterial(
            const MaterialInfo& info,
            const std::string& baseDirectory,
            const std::unordered_map<std::string, std::shared_ptr<Texture>>& loadedTextures);

        /**
         * @brief Extract texture paths from material info
         */
        static std::vector<std::string> extractTexturePaths(
            const MaterialInfo& info, 
            const std::string& baseDirectory);

    private:
        /**
         * @brief Parse a single line from MTL file
         */
        static void parseMTLLine(const std::string& line, MaterialInfo& material);
    };

    /**
     * @brief Manages dependencies for a group of assets
     */
    class DependencyGraph {
    public:
        using LoadCallback = std::function<void(bool success, const std::string& error)>;

        /**
         * @brief Add a dependency to the graph
         */
        void addDependency(std::shared_ptr<AssetDependency> dependency);

        /**
         * @brief Start loading all dependencies asynchronously
         */
        void loadDependencies(Vulkan::VulkanDevice& device, LoadCallback callback);

        /**
         * @brief Check if all dependencies are loaded
         */
        bool isFullyLoaded() const;

        /**
         * @brief Get loaded asset by path
         */
        std::shared_ptr<void> getAsset(const std::string& path) const;

        /**
         * @brief Get loaded texture by path
         */
        std::shared_ptr<Texture> getTexture(const std::string& path) const;

        /**
         * @brief Get loaded material by name
         */
        std::shared_ptr<UnifiedMaterialInstance> getMaterial(const std::string& name) const;

        /**
         * @brief Get all dependencies
         */
        const std::vector<std::shared_ptr<AssetDependency>>& getDependencies() const { return m_dependencies; }

        /**
         * @brief Get loading progress (0.0 to 1.0)
         */
        float getLoadingProgress() const;

    private:
        std::vector<std::shared_ptr<AssetDependency>> m_dependencies;
        std::unordered_map<std::string, std::shared_ptr<AssetDependency>> m_dependencyMap;
        
        void loadTextureDependency(std::shared_ptr<AssetDependency> dep, Vulkan::VulkanDevice& device);
        void checkLoadingComplete(LoadCallback callback);
    };

} // namespace AstralEngine
