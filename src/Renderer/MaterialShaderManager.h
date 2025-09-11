#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include <string>

// Forward declarations
namespace AstralEngine {
    namespace Vulkan {
        class VulkanDevice;
    }
    class Shader;
    class UnifiedMaterialInstance;
}

namespace AstralEngine {

    /**
     * @brief Manages shader compilation and caching for different material variants
     * 
     * The MaterialShaderManager is responsible for generating, compiling, and caching
     * shader variants based on material properties. It handles dynamic shader
     * compilation with different defines and preprocessor flags.
     */
     class MaterialShaderManager {
    public:
        explicit MaterialShaderManager(Vulkan::VulkanDevice& device);
        ~MaterialShaderManager() = default;

        // Non-copyable and non-movable
        MaterialShaderManager(const MaterialShaderManager&) = delete;
        MaterialShaderManager& operator=(const MaterialShaderManager&) = delete;
        MaterialShaderManager(MaterialShaderManager&&) = delete;
        MaterialShaderManager& operator=(MaterialShaderManager&&) = delete;

        /**
         * @brief Get or compile a shader for the given material
         * @param material The material instance that determines shader variant
         * @return Shared pointer to the compiled shader, or nullptr if compilation failed
         */
        std::shared_ptr<Shader> getShader(const UnifiedMaterialInstance& material);

        /**
         * @brief Get or compile a shader for the given variant string
         * @param variant Shader variant defines (space-separated)
         * @return Shared pointer to the compiled shader, or nullptr if compilation failed
         */
        std::shared_ptr<Shader> getShader(const std::string& variant);

        // Offline build: no runtime precompilation
        void precompileCommonVariants() = delete;

        // Hot-reload functionality removed (offline compilation only)

        /**
         * @brief Reload all cached shaders (useful for development)
         */
        void reloadShaders() = delete; // Hot reload disabled in offline-only pipeline

        // Shader base paths now hardcoded for offline build (unified_pbr.vert + variants for .frag)

        /**
         * @brief Get the number of compiled shader variants
         * @return Number of cached shaders
         */
        size_t getCachedShaderCount() const { return m_shaderCache.size(); }

        /**
         * @brief Clear all cached shaders
         */
        void clearCache() { m_shaderCache.clear(); }

        /**
         * @brief Print debug statistics about the shader manager
         */
        void printStatistics() const;

    private:
        /**
         * @brief Generate shader preprocessor defines for a material
         * @param material The material instance to generate defines for
         * @return String containing all shader defines
         */
        std::string generateShaderDefines(const UnifiedMaterialInstance& material) const;

        /**
         * @brief Compile a shader variant with the given defines
         * @param variant Shader variant defines string
         * @return Compiled shader or nullptr if compilation failed
         */
        std::shared_ptr<Shader> createShaderFromVariant(const std::string& variant);
        std::string pickFragmentShaderBaseFromVariant(const std::string& variant) const;

        // Device reference
        Vulkan::VulkanDevice& m_device;

        // Shader cache: variant string -> compiled shader (loaded from offline SPIR-V)
        std::unordered_map<std::string, std::shared_ptr<Shader>> m_shaderCache;

        // Common defines that are added to all shaders
        static const std::vector<std::string> s_commonDefines;
    };

} // namespace AstralEngine
