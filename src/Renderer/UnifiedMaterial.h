#pragma once

#include "UnifiedMaterialConstants.h"
#include "Texture.h"
#include <memory>
#include <array>
#include <unordered_map>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

// Forward declarations
namespace AstralEngine {
    struct PipelineConfig;  // From PipelineConfig.h
    
    namespace Vulkan {
        class VulkanDevice;
        class VulkanBuffer;
        class DescriptorSetManager;
    }
}

namespace AstralEngine {

    /**
     * Unified Material Instance
     * Combines and replaces both MaterialInstance and PBRMaterialInstance
     * Provides backward compatibility while supporting modern PBR workflows
     */
    class UnifiedMaterialInstance {
    public:
        // Constructors
        UnifiedMaterialInstance();
        explicit UnifiedMaterialInstance(const MaterialTemplate& materialTemplate);
        ~UnifiedMaterialInstance();

        // Move-only semantics for efficient resource management
        UnifiedMaterialInstance(const UnifiedMaterialInstance&) = delete;
        UnifiedMaterialInstance& operator=(const UnifiedMaterialInstance&) = delete;
        UnifiedMaterialInstance(UnifiedMaterialInstance&& other) noexcept;
        UnifiedMaterialInstance& operator=(UnifiedMaterialInstance&& other) noexcept;

        // === Material Properties ===
        
        // Direct UBO access
        const UnifiedMaterialUBO& getUBO() const { return m_ubo; }
        UnifiedMaterialUBO& getUBO() { return m_ubo; }
        void markDirty() { m_dirty = true; }
        bool isDirty() const { return m_dirty; }
        void markClean() { m_dirty = false; }

        // Convenience setters for common properties
        void setBaseColor(const glm::vec4& color);
        void setMetallic(float metallic);
        void setRoughness(float roughness);
        void setNormalScale(float scale);
        void setEmissive(const glm::vec3& color, float intensity = 1.0f);
        void setSpecular(const glm::vec3& specular, float factor = 1.0f);

        // Advanced PBR properties
        void setAnisotropy(float anisotropy, float rotation = 0.0f);
        void setClearcoat(float clearcoat, float roughness = 0.0f, float normalScale = 1.0f);
        void setSheen(float sheen, const glm::vec3& color = glm::vec3(0.0f), float roughness = 0.0f);
        void setTransmission(float transmission, float ior = 1.5f, float thickness = 0.0f);
        
        // Alpha blending
        void setAlphaMode(AlphaMode mode, float cutoff = 0.5f);
        void setOpaque() { setAlphaMode(AlphaMode::Opaque); }
        void setAlphaTest(float cutoff = 0.5f) { setAlphaMode(AlphaMode::Mask, cutoff); }
        void setTransparent() { setAlphaMode(AlphaMode::Blend); }

        // Getters
        glm::vec4 getBaseColor() const { return m_ubo.baseColor; }
        float getMetallic() const { return m_ubo.metallic; }
        float getRoughness() const { return m_ubo.roughness; }
        glm::vec3 getEmissive() const { return m_ubo.emissiveColor; }
        AlphaMode getAlphaMode() const { return ::AstralEngine::getAlphaMode(m_ubo); }
        float getAlphaCutoff() const { return m_ubo.getAlphaCutoff(); }

        // === Texture Management ===
        
        void setTexture(TextureSlot slot, std::shared_ptr<Texture> texture);
        std::shared_ptr<Texture> getTexture(TextureSlot slot) const;
        void removeTexture(TextureSlot slot);
        bool hasTexture(TextureSlot slot) const;

        // Legacy string-based texture access (for backward compatibility)
        void setTexture(const std::string& name, std::shared_ptr<Texture> texture);
        std::shared_ptr<Texture> getTexture(const std::string& name) const;

        // Texture coordinate transforms
        void setTextureTransform(TextureSlot slot, const glm::vec2& tiling, const glm::vec2& offset);
        void setTextureTransform(TextureSlot slot, float tilingU, float tilingV, float offsetU = 0.0f, float offsetV = 0.0f);

        // === Material Features and Flags ===
        
        void setTextureFlag(TextureFlags flag, bool enabled);
        void setFeatureFlag(FeatureFlags flag, bool enabled);
        void setRenderingFlag(RenderingFlags flag, bool enabled);
        
        bool hasTextureFlag(TextureFlags flag) const;
        bool hasFeatureFlag(FeatureFlags flag) const;
        bool hasRenderingFlag(RenderingFlags flag) const;

        // === Rendering Properties ===
        
        bool isTransparent() const;
        bool isOpaque() const { return !isTransparent(); }
        bool needsAlphaBlending() const;
        bool castsShadows() const;
        bool receivesShadows() const;
        bool isUnlit() const;
        bool isDoubleSided() const;

        // Render queue determination
        int getRenderQueue() const;

        // === Shader System Integration ===
        
        std::string getShaderVariant() const;
        uint64_t getVariantHash() const;
        
        // === Pipeline Configuration ===
        
        /**
         * Derives the appropriate pipeline configuration from current material properties.
         * This enables efficient pipeline caching based on material render state.
         */
        PipelineConfig getPipelineConfig() const;

        // === Vulkan Descriptor Management ===
        
        void buildDescriptorSets(
            Vulkan::VulkanDevice& device,
            VkDescriptorSetLayout materialSetLayout,
            Vulkan::DescriptorSetManager& descManager,
            VkImageView fallbackView,
            VkSampler fallbackSampler,
            size_t imageCount);

        VkDescriptorSet getDescriptorSet(size_t imageIndex) const;
        bool isDescriptorSetBuilt() const { return !m_descriptorSets.empty(); }
        bool needsDescriptorSetRebuild(size_t imageCount) const { return m_descriptorSets.size() != imageCount; }
        void destroyDescriptorSets();

        // Apply UBO data to GPU buffers
        void applyUBOToGPU(Vulkan::VulkanDevice& device, size_t imageIndex);

        // === Template System ===
        
        void setTemplate(const MaterialTemplate& materialTemplate);
        const MaterialTemplate& getTemplate() const { return m_template; }
        bool hasTemplate() const { return m_hasTemplate; }

        // === Factory Methods (for common material types) ===
        
        static UnifiedMaterialInstance createDefault();
        static UnifiedMaterialInstance createMetal(const glm::vec3& baseColor, float roughness = 0.1f);
        static UnifiedMaterialInstance createDielectric(const glm::vec3& baseColor, float roughness = 0.5f);
        static UnifiedMaterialInstance createPlastic(const glm::vec3& baseColor, float roughness = 0.3f);
        static UnifiedMaterialInstance createGlass(const glm::vec3& baseColor, float transmission = 1.0f);
        static UnifiedMaterialInstance createEmissive(const glm::vec3& emissiveColor, float intensity = 1.0f);

        // === Legacy Compatibility ===
        
        // Note: Legacy API is now handled directly by the main interface above

    private:
        // Core data
        UnifiedMaterialUBO m_ubo;
        bool m_dirty = true;
        
        // Template system
        MaterialTemplate m_template;
        bool m_hasTemplate = false;

        // Texture storage
        std::array<std::shared_ptr<Texture>, static_cast<size_t>(TextureSlot::Count)> m_textures;
        std::unordered_map<std::string, TextureSlot> m_legacyTextureMapping; // For backward compatibility

        // Vulkan resources
        std::vector<VkDescriptorSet> m_descriptorSets;
        std::vector<std::unique_ptr<Vulkan::VulkanBuffer>> m_uboBuffers;  // RAII-compliant smart pointers
        Vulkan::DescriptorSetManager* m_descriptorManager = nullptr; // Non-owning pointer for cleanup

        // Cached shader variant data
        mutable std::string m_cachedVariant;
        mutable uint64_t m_cachedVariantHash = 0;
        mutable bool m_variantCacheDirty = true;

        // Internal methods
        void updateTextureFlags();
        void updateFeatureFlags();
        void updateTextureIndices();
        void applyTemplate();
        void invalidateVariantCache() const { m_variantCacheDirty = true; }
        TextureSlot stringToTextureSlot(const std::string& name) const;
        std::string generateShaderDefines() const;

        // Move helper
        void moveFrom(UnifiedMaterialInstance&& other);
    };

    // === Backward Compatibility Aliases ===
    
    // For existing code that uses the old names
    using MaterialInstance = UnifiedMaterialInstance;
    using PBRMaterialInstance = UnifiedMaterialInstance;

    // Factory class for creating materials (replaces PBRMaterialFactory)
    class MaterialFactory {
    public:
        // Create predefined material types
        static UnifiedMaterialInstance createDefaultMaterial() { return UnifiedMaterialInstance::createDefault(); }
        static UnifiedMaterialInstance createMetalMaterial(const glm::vec3& baseColor, float roughness = 0.1f) { 
            return UnifiedMaterialInstance::createMetal(baseColor, roughness); 
        }
        static UnifiedMaterialInstance createDielectricMaterial(const glm::vec3& baseColor, float roughness = 0.5f) { 
            return UnifiedMaterialInstance::createDielectric(baseColor, roughness); 
        }
        static UnifiedMaterialInstance createPlasticMaterial(const glm::vec3& baseColor, float roughness = 0.3f) { 
            return UnifiedMaterialInstance::createPlastic(baseColor, roughness); 
        }
        static UnifiedMaterialInstance createGlassMaterial(const glm::vec3& baseColor, float transmission = 1.0f) { 
            return UnifiedMaterialInstance::createGlass(baseColor, transmission); 
        }
        static UnifiedMaterialInstance createEmissiveMaterial(const glm::vec3& emissiveColor, float intensity = 1.0f) { 
            return UnifiedMaterialInstance::createEmissive(emissiveColor, intensity); 
        }

        // Material templates
        static UnifiedMaterialInstance createFromTemplate(const std::string& templateName);
        static void registerTemplate(const std::string& name, const MaterialTemplate& materialTemplate);

    private:
        static std::unordered_map<std::string, MaterialTemplate> s_templates;
    };

    // Backward compatibility alias
    using PBRMaterialFactory = MaterialFactory;

} // namespace AstralEngine
