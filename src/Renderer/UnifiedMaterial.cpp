#include "UnifiedMaterial.h"
#include "Renderer/PipelineConfig.h"
#include "Renderer/VulkanR/VulkanDevice.h"
#include "Renderer/VulkanR/VulkanBuffer.h"
#include "Renderer/VulkanR/DescriptorSetManager.h"
#include "Core/Logger.h"
#include <cstring>
#include <sstream>
#include <algorithm>
#include <memory>

namespace AstralEngine {

    // Static template storage
    std::unordered_map<std::string, MaterialTemplate> MaterialFactory::s_templates;

    // === UnifiedMaterialInstance Implementation ===

    UnifiedMaterialInstance::UnifiedMaterialInstance() {
        // Initialize legacy texture mapping for backward compatibility
        m_legacyTextureMapping["albedo"] = TextureSlot::BaseColor;
        m_legacyTextureMapping["baseColor"] = TextureSlot::BaseColor;
        m_legacyTextureMapping["diffuse"] = TextureSlot::BaseColor;
        m_legacyTextureMapping["normal"] = TextureSlot::Normal;
        m_legacyTextureMapping["normalMap"] = TextureSlot::Normal;
        m_legacyTextureMapping["metallic"] = TextureSlot::MetallicRoughness;
        m_legacyTextureMapping["metallicRoughness"] = TextureSlot::MetallicRoughness;
        m_legacyTextureMapping["roughness"] = TextureSlot::MetallicRoughness;
        m_legacyTextureMapping["occlusion"] = TextureSlot::Occlusion;
        m_legacyTextureMapping["ao"] = TextureSlot::Occlusion;
        m_legacyTextureMapping["emissive"] = TextureSlot::Emissive;
        m_legacyTextureMapping["emission"] = TextureSlot::Emissive;
    }

    UnifiedMaterialInstance::UnifiedMaterialInstance(const MaterialTemplate& materialTemplate) 
        : UnifiedMaterialInstance() {
        setTemplate(materialTemplate);
    }

    UnifiedMaterialInstance::~UnifiedMaterialInstance() {
        destroyDescriptorSets();
    }

    UnifiedMaterialInstance::UnifiedMaterialInstance(UnifiedMaterialInstance&& other) noexcept {
        moveFrom(std::move(other));
    }

    UnifiedMaterialInstance& UnifiedMaterialInstance::operator=(UnifiedMaterialInstance&& other) noexcept {
        if (this != &other) {
            destroyDescriptorSets();
            moveFrom(std::move(other));
        }
        return *this;
    }

    void UnifiedMaterialInstance::moveFrom(UnifiedMaterialInstance&& other) {
        // Move core data
        m_ubo = std::move(other.m_ubo);
        m_dirty = other.m_dirty;
        
        // Move template data
        m_template = std::move(other.m_template);
        m_hasTemplate = other.m_hasTemplate;
        
        // Move textures
        m_textures = std::move(other.m_textures);
        m_legacyTextureMapping = std::move(other.m_legacyTextureMapping);
        
        // Move Vulkan resources
        m_descriptorSets = std::move(other.m_descriptorSets);
        m_uboBuffers = std::move(other.m_uboBuffers);
        m_descriptorManager = other.m_descriptorManager;
        
        // Move cached data
        m_cachedVariant = std::move(other.m_cachedVariant);
        m_cachedVariantHash = other.m_cachedVariantHash;
        m_variantCacheDirty = other.m_variantCacheDirty;
        
        // Reset other object
        other.m_descriptorSets.clear();
        other.m_uboBuffers.clear();
        other.m_descriptorManager = nullptr;
        other.m_dirty = true;
        other.m_hasTemplate = false;
        other.m_variantCacheDirty = true;
    }

    // === Property Setters ===

    void UnifiedMaterialInstance::setBaseColor(const glm::vec4& color) {
        m_ubo.baseColor = color;
        markDirty();
        invalidateVariantCache();
    }

    void UnifiedMaterialInstance::setMetallic(float metallic) {
        m_ubo.metallic = glm::clamp(metallic, 0.0f, 1.0f);
        updateFeatureFlags();
        markDirty();
        invalidateVariantCache();
    }

    void UnifiedMaterialInstance::setRoughness(float roughness) {
        m_ubo.roughness = glm::clamp(roughness, 0.0f, 1.0f);
        markDirty();
        invalidateVariantCache();
    }

    void UnifiedMaterialInstance::setNormalScale(float scale) {
        m_ubo.getNormalScale() = scale;
        markDirty();
        invalidateVariantCache();
    }

    void UnifiedMaterialInstance::setEmissive(const glm::vec3& color, float intensity) {
        m_ubo.emissiveColor = color * intensity;
        updateFeatureFlags();
        markDirty();
        invalidateVariantCache();
    }

    void UnifiedMaterialInstance::setSpecular(const glm::vec3& specular, float factor) {
        m_ubo.specularColor = specular;
        m_ubo.getSpecularFactor() = factor;
        setFeatureFlag(FeatureFlags::UseSpecular, glm::length(specular) > 0.01f || factor != 1.0f);
        markDirty();
        invalidateVariantCache();
    }

    void UnifiedMaterialInstance::setAnisotropy(float anisotropy, float rotation) {
        m_ubo.getAnisotropy() = glm::clamp(anisotropy, 0.0f, 1.0f);
        m_ubo.getAnisotropyRotation() = rotation;
        setFeatureFlag(FeatureFlags::UseAnisotropy, anisotropy > 0.01f);
        markDirty();
        invalidateVariantCache();
    }

    void UnifiedMaterialInstance::setClearcoat(float clearcoat, float roughness, float normalScale) {
        m_ubo.getClearcoat() = glm::clamp(clearcoat, 0.0f, 1.0f);
        m_ubo.getClearcoatRoughness() = glm::clamp(roughness, 0.0f, 1.0f);
        m_ubo.clearcoatNormalScale = normalScale;
        setFeatureFlag(FeatureFlags::UseClearcoat, clearcoat > 0.01f);
        markDirty();
        invalidateVariantCache();
    }

    void UnifiedMaterialInstance::setSheen(float sheen, const glm::vec3& color, float roughness) {
        m_ubo.getSheen() = glm::clamp(sheen, 0.0f, 1.0f);
        m_ubo.sheenColor = color;
        m_ubo.getSheenRoughness() = glm::clamp(roughness, 0.0f, 1.0f);
        setFeatureFlag(FeatureFlags::UseSheen, sheen > 0.01f || glm::length(color) > 0.01f);
        markDirty();
        invalidateVariantCache();
    }

    void UnifiedMaterialInstance::setTransmission(float transmission, float ior, float thickness) {
        m_ubo.getTransmission() = glm::clamp(transmission, 0.0f, 1.0f);
        m_ubo.getIor() = glm::clamp(ior, 1.0f, 3.0f);
        m_ubo.getThickness() = glm::max(thickness, 0.0f);
        setFeatureFlag(FeatureFlags::UseTransmission, transmission > 0.01f);
        updateFeatureFlags(); // This might affect transparency
        markDirty();
        invalidateVariantCache();
    }

    void UnifiedMaterialInstance::setAlphaMode(AlphaMode mode, float cutoff) {
        ::AstralEngine::setAlphaMode(m_ubo, mode);
        m_ubo.getAlphaCutoff() = cutoff;
        
        // Update rendering flags based on alpha mode
        setRenderingFlag(RenderingFlags::AlphaTest, mode == AlphaMode::Mask);
        setRenderingFlag(RenderingFlags::AlphaBlend, mode == AlphaMode::Blend);
        
        markDirty();
        invalidateVariantCache();
    }

    // === Texture Management ===

    void UnifiedMaterialInstance::setTexture(TextureSlot slot, std::shared_ptr<Texture> texture) {
        if (slot >= TextureSlot::Count) {
            AE_ERROR("Invalid texture slot: {}", static_cast<uint32_t>(slot));
            return;
        }

        size_t slotIndex = static_cast<size_t>(slot);
        m_textures[slotIndex] = texture;
        
        // Update texture flags and indices
        updateTextureFlags();
        updateTextureIndices();
        
        markDirty();
        invalidateVariantCache();
    }

    std::shared_ptr<Texture> UnifiedMaterialInstance::getTexture(TextureSlot slot) const {
        if (slot >= TextureSlot::Count) {
            return nullptr;
        }
        return m_textures[static_cast<size_t>(slot)];
    }

    void UnifiedMaterialInstance::removeTexture(TextureSlot slot) {
        setTexture(slot, nullptr);
    }

    bool UnifiedMaterialInstance::hasTexture(TextureSlot slot) const {
        return getTexture(slot) != nullptr;
    }

    void UnifiedMaterialInstance::setTexture(const std::string& name, std::shared_ptr<Texture> texture) {
        TextureSlot slot = stringToTextureSlot(name);
        if (slot != TextureSlot::Count) {
            setTexture(slot, texture);
        } else {
            AE_WARN("Unknown texture slot name: {}", name);
        }
    }

    std::shared_ptr<Texture> UnifiedMaterialInstance::getTexture(const std::string& name) const {
        TextureSlot slot = stringToTextureSlot(name);
        if (slot != TextureSlot::Count) {
            return getTexture(slot);
        }
        return nullptr;
    }

    void UnifiedMaterialInstance::setTextureTransform(TextureSlot slot, const glm::vec2& tiling, const glm::vec2& offset) {
        switch (slot) {
            case TextureSlot::BaseColor:
                m_ubo.getBaseColorTiling() = tiling;
                m_ubo.getBaseColorOffset() = offset;
                break;
            case TextureSlot::Normal:
                m_ubo.getNormalTiling() = tiling;
                m_ubo.getNormalOffset() = offset;
                break;
            case TextureSlot::MetallicRoughness:
                m_ubo.getMetallicRoughnessTiling() = tiling;
                m_ubo.getMetallicRoughnessOffset() = offset;
                break;
            case TextureSlot::Emissive:
                m_ubo.getEmissiveTiling() = tiling;
                m_ubo.getEmissiveOffset() = offset;
                break;
            default:
                AE_WARN("Texture transform not supported for slot: {}", static_cast<uint32_t>(slot));
                break;
        }
        markDirty();
    }

    void UnifiedMaterialInstance::setTextureTransform(TextureSlot slot, float tilingU, float tilingV, float offsetU, float offsetV) {
        setTextureTransform(slot, glm::vec2(tilingU, tilingV), glm::vec2(offsetU, offsetV));
    }

    // === Flag Management ===

    void UnifiedMaterialInstance::setTextureFlag(TextureFlags flag, bool enabled) {
        ::AstralEngine::setTextureFlag(m_ubo, flag, enabled);
        markDirty();
        invalidateVariantCache();
    }

    void UnifiedMaterialInstance::setFeatureFlag(FeatureFlags flag, bool enabled) {
        ::AstralEngine::setFeatureFlag(m_ubo, flag, enabled);
        markDirty();
        invalidateVariantCache();
    }

    void UnifiedMaterialInstance::setRenderingFlag(RenderingFlags flag, bool enabled) {
        ::AstralEngine::setRenderingFlag(m_ubo, flag, enabled);
        markDirty();
        invalidateVariantCache();
    }

    bool UnifiedMaterialInstance::hasTextureFlag(TextureFlags flag) const {
        return ::AstralEngine::hasTextureFlag(m_ubo, flag);
    }

    bool UnifiedMaterialInstance::hasFeatureFlag(FeatureFlags flag) const {
        return ::AstralEngine::hasFeatureFlag(m_ubo, flag);
    }

    bool UnifiedMaterialInstance::hasRenderingFlag(RenderingFlags flag) const {
        return ::AstralEngine::hasRenderingFlag(m_ubo, flag);
    }

    // === Rendering Properties ===

    bool UnifiedMaterialInstance::isTransparent() const {
        AlphaMode mode = getAlphaMode();
        return mode == AlphaMode::Blend || 
               (mode == AlphaMode::Mask && m_ubo.baseColor.a < 1.0f) ||
               m_ubo.getTransmission() > 0.01f;
    }

    bool UnifiedMaterialInstance::needsAlphaBlending() const {
        return hasRenderingFlag(RenderingFlags::AlphaBlend) || 
               getAlphaMode() == AlphaMode::Blend;
    }

    bool UnifiedMaterialInstance::castsShadows() const {
        return hasFeatureFlag(FeatureFlags::CastShadows) && 
               !isTransparent() && // Transparent objects typically don't cast hard shadows
               !hasFeatureFlag(FeatureFlags::IsUnlit);
    }

    bool UnifiedMaterialInstance::receivesShadows() const {
        return hasFeatureFlag(FeatureFlags::ReceiveShadows) && 
               !hasFeatureFlag(FeatureFlags::IsUnlit);
    }

    bool UnifiedMaterialInstance::isUnlit() const {
        return hasFeatureFlag(FeatureFlags::IsUnlit);
    }

    bool UnifiedMaterialInstance::isDoubleSided() const {
        return hasFeatureFlag(FeatureFlags::IsDoubleSided);
    }

    int UnifiedMaterialInstance::getRenderQueue() const {
        if (isTransparent()) {
            return 3000; // Transparent queue
        }
        
        if (getAlphaMode() == AlphaMode::Mask) {
            return 2500; // Alpha test queue
        }
        
        return 2000; // Opaque queue
    }

    // === Shader System Integration ===

    std::string UnifiedMaterialInstance::getShaderVariant() const {
        if (!m_variantCacheDirty && !m_cachedVariant.empty()) {
            return m_cachedVariant;
        }
        
        m_cachedVariant = generateShaderDefines();
        m_variantCacheDirty = false;
        return m_cachedVariant;
    }

    uint64_t UnifiedMaterialInstance::getVariantHash() const {
        if (!m_variantCacheDirty && m_cachedVariantHash != 0) {
            return m_cachedVariantHash;
        }
        
        // Generate hash from flags and key properties
        std::hash<uint32_t> hasher;
        uint64_t hash = 0;
        
        // Hash material flags
        hash ^= hasher(m_ubo.materialFlags.x) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= hasher(m_ubo.materialFlags.y) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= hasher(m_ubo.materialFlags.z) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= hasher(m_ubo.materialFlags.w) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        
        // Hash key feature usage
        if (m_ubo.getAnisotropy() > 0.01f) hash ^= 0x1;
        if (m_ubo.getClearcoat() > 0.01f) hash ^= 0x2;
        if (m_ubo.getSheen() > 0.01f) hash ^= 0x4;
        if (m_ubo.getTransmission() > 0.01f) hash ^= 0x8;
        
        m_cachedVariantHash = hash;
        m_variantCacheDirty = false;
        return hash;
    }
    
    // === Pipeline Configuration ===
    
    PipelineConfig UnifiedMaterialInstance::getPipelineConfig() const {
        PipelineConfig config = PipelineConfig::createDefault();
        
        // Handle double-sided materials
        if (isDoubleSided()) {
            config.rasterization.cullMode = VK_CULL_MODE_NONE;
        }
        
        const AlphaMode alphaMode = getAlphaMode();
        
        // Handle transparent and alpha-blended materials
        if (needsAlphaBlending() || isTransparent()) {
            // Use transparent configuration: depth test on, depth write off, alpha blending on
            config = PipelineConfig::createTransparent();
            
            // Override culling if double-sided
            if (isDoubleSided()) {
                config.rasterization.cullMode = VK_CULL_MODE_NONE;
            }
            
            // Optional: For transmission-heavy materials, could use additive blending
            // based on feature flags, but standard alpha blend is good for most cases
        }
        else if (alphaMode == AlphaMode::Mask) {
            // Alpha test: depth test/write enabled, blending disabled
            config.depthStencil = DepthStencilConfig::createDefault();
            config.blend = BlendConfig::createOpaque();
        }
        else {
            // Opaque materials: use defaults
            config.depthStencil = DepthStencilConfig::createDefault();
            config.blend = BlendConfig::createOpaque();
        }
        
        // Future: Could apply template-specific modifications here
        // if (m_hasTemplate && m_template.hasWireframe()) {
        //     config.rasterization.polygonMode = VK_POLYGON_MODE_LINE;
        // }
        
        return config;
    }
    
    // === Internal Methods ===

    void UnifiedMaterialInstance::updateTextureFlags() {
        // Update texture flags based on which textures are actually present
        setTextureFlag(TextureFlags::HasBaseColor, hasTexture(TextureSlot::BaseColor));
        setTextureFlag(TextureFlags::HasMetallicRoughness, hasTexture(TextureSlot::MetallicRoughness));
        setTextureFlag(TextureFlags::HasNormal, hasTexture(TextureSlot::Normal));
        setTextureFlag(TextureFlags::HasOcclusion, hasTexture(TextureSlot::Occlusion));
        setTextureFlag(TextureFlags::HasEmissive, hasTexture(TextureSlot::Emissive));
        setTextureFlag(TextureFlags::HasAnisotropy, hasTexture(TextureSlot::Anisotropy));
        setTextureFlag(TextureFlags::HasClearcoat, hasTexture(TextureSlot::Clearcoat));
        setTextureFlag(TextureFlags::HasClearcoatNormal, hasTexture(TextureSlot::ClearcoatNormal));
        setTextureFlag(TextureFlags::HasClearcoatRoughness, hasTexture(TextureSlot::ClearcoatRoughness));
        setTextureFlag(TextureFlags::HasSheen, hasTexture(TextureSlot::Sheen));
        setTextureFlag(TextureFlags::HasSheenColor, hasTexture(TextureSlot::SheenColor));
        setTextureFlag(TextureFlags::HasSheenRoughness, hasTexture(TextureSlot::SheenRoughness));
        setTextureFlag(TextureFlags::HasTransmission, hasTexture(TextureSlot::Transmission));
        setTextureFlag(TextureFlags::HasVolume, hasTexture(TextureSlot::Volume));
        setTextureFlag(TextureFlags::HasSpecular, hasTexture(TextureSlot::Specular));
        setTextureFlag(TextureFlags::HasSpecularColor, hasTexture(TextureSlot::SpecularColor));
    }

    void UnifiedMaterialInstance::updateFeatureFlags() {
        // Update feature flags based on material properties
        setFeatureFlag(FeatureFlags::UseAnisotropy, m_ubo.getAnisotropy() > 0.01f);
        setFeatureFlag(FeatureFlags::UseClearcoat, m_ubo.getClearcoat() > 0.01f);
        setFeatureFlag(FeatureFlags::UseSheen, m_ubo.getSheen() > 0.01f || glm::length(m_ubo.sheenColor) > 0.01f);
        setFeatureFlag(FeatureFlags::UseTransmission, m_ubo.getTransmission() > 0.01f);
        setFeatureFlag(FeatureFlags::UseVolume, m_ubo.getVolume() > 0.01f);
        setFeatureFlag(FeatureFlags::UseSpecular, glm::length(m_ubo.specularColor) > 0.01f || m_ubo.getSpecularFactor() != 1.0f);
        
        // Set default shadow behavior
        if (!hasFeatureFlag(FeatureFlags::IsUnlit)) {
            setFeatureFlag(FeatureFlags::CastShadows, !isTransparent());
            setFeatureFlag(FeatureFlags::ReceiveShadows, true);
        }
    }

    void UnifiedMaterialInstance::updateTextureIndices() {
        // Update texture indices in the UBO (for bindless textures)
        // For now, we'll use invalid indices (~0u) since we're using traditional binding
        const uint32_t invalidIndex = ~0u;
        
        for (uint32_t i = 0; i < static_cast<uint32_t>(TextureSlot::Count); ++i) {
            TextureSlot slot = static_cast<TextureSlot>(i);
            setTextureIndex(m_ubo, slot, invalidIndex); // Will be set by renderer during binding
        }
    }

    void UnifiedMaterialInstance::applyTemplate() {
        if (m_hasTemplate) {
            m_ubo = m_template.applyTemplate(m_ubo);
        }
    }

    TextureSlot UnifiedMaterialInstance::stringToTextureSlot(const std::string& name) const {
        auto it = m_legacyTextureMapping.find(name);
        if (it != m_legacyTextureMapping.end()) {
            return it->second;
        }
        return TextureSlot::Count; // Invalid slot
    }

    std::string UnifiedMaterialInstance::generateShaderDefines() const {
        std::ostringstream defines;
        
        // Texture defines
        if (hasTextureFlag(TextureFlags::HasBaseColor)) defines << "HAS_BASECOLOR_MAP ";
        if (hasTextureFlag(TextureFlags::HasMetallicRoughness)) defines << "HAS_METALLIC_ROUGHNESS_MAP ";
        if (hasTextureFlag(TextureFlags::HasNormal)) defines << "HAS_NORMAL_MAP ";
        if (hasTextureFlag(TextureFlags::HasOcclusion)) defines << "HAS_OCCLUSION_MAP ";
        if (hasTextureFlag(TextureFlags::HasEmissive)) defines << "HAS_EMISSIVE_MAP ";
        
        // Advanced texture defines
        if (hasTextureFlag(TextureFlags::HasAnisotropy)) defines << "HAS_ANISOTROPY_MAP ";
        if (hasTextureFlag(TextureFlags::HasClearcoat)) defines << "HAS_CLEARCOAT_MAP ";
        if (hasTextureFlag(TextureFlags::HasClearcoatNormal)) defines << "HAS_CLEARCOAT_NORMAL_MAP ";
        if (hasTextureFlag(TextureFlags::HasClearcoatRoughness)) defines << "HAS_CLEARCOAT_ROUGHNESS_MAP ";
        if (hasTextureFlag(TextureFlags::HasSheen)) defines << "HAS_SHEEN_MAP ";
        if (hasTextureFlag(TextureFlags::HasSheenColor)) defines << "HAS_SHEEN_COLOR_MAP ";
        if (hasTextureFlag(TextureFlags::HasSheenRoughness)) defines << "HAS_SHEEN_ROUGHNESS_MAP ";
        if (hasTextureFlag(TextureFlags::HasTransmission)) defines << "HAS_TRANSMISSION_MAP ";
        if (hasTextureFlag(TextureFlags::HasVolume)) defines << "HAS_VOLUME_MAP ";
        if (hasTextureFlag(TextureFlags::HasSpecular)) defines << "HAS_SPECULAR_MAP ";
        if (hasTextureFlag(TextureFlags::HasSpecularColor)) defines << "HAS_SPECULAR_COLOR_MAP ";
        
        // Feature defines
        if (hasFeatureFlag(FeatureFlags::UseAnisotropy)) defines << "USE_ANISOTROPY ";
        if (hasFeatureFlag(FeatureFlags::UseClearcoat)) defines << "USE_CLEARCOAT ";
        if (hasFeatureFlag(FeatureFlags::UseSheen)) defines << "USE_SHEEN ";
        if (hasFeatureFlag(FeatureFlags::UseTransmission)) defines << "USE_TRANSMISSION ";
        if (hasFeatureFlag(FeatureFlags::UseVolume)) defines << "USE_VOLUME ";
        if (hasFeatureFlag(FeatureFlags::UseSpecular)) defines << "USE_SPECULAR ";
        if (hasFeatureFlag(FeatureFlags::UseVertexColors)) defines << "USE_VERTEX_COLORS ";
        if (hasFeatureFlag(FeatureFlags::UseSecondaryUV)) defines << "USE_SECONDARY_UV ";
        if (hasFeatureFlag(FeatureFlags::IsUnlit)) defines << "UNLIT ";
        if (hasFeatureFlag(FeatureFlags::IsDoubleSided)) defines << "DOUBLE_SIDED ";
        if (hasFeatureFlag(FeatureFlags::ReceiveShadows)) defines << "RECEIVE_SHADOWS ";
        
        // Alpha mode defines
        AlphaMode alphaMode = getAlphaMode();
        if (alphaMode == AlphaMode::Mask) defines << "ALPHA_TEST ";
        else if (alphaMode == AlphaMode::Blend) defines << "ALPHA_BLEND ";
        
        // Rendering defines
        if (hasRenderingFlag(RenderingFlags::AlphaTest)) defines << "ALPHA_TEST ";
        if (hasRenderingFlag(RenderingFlags::AlphaBlend)) defines << "ALPHA_BLEND ";
        
        return defines.str();
    }

    // === Template System ===

    void UnifiedMaterialInstance::setTemplate(const MaterialTemplate& materialTemplate) {
        m_template = materialTemplate;
        m_hasTemplate = true;
        applyTemplate();
        markDirty();
        invalidateVariantCache();
    }

    // === Vulkan Descriptor Management ===

    void UnifiedMaterialInstance::buildDescriptorSets(
        Vulkan::VulkanDevice& device,
        VkDescriptorSetLayout materialSetLayout,
        Vulkan::DescriptorSetManager& descManager,
        VkImageView fallbackView,
        VkSampler fallbackSampler,
        size_t imageCount) {
        
        // Clean up existing resources
        destroyDescriptorSets();
        
        m_descriptorSets.resize(imageCount);
        m_uboBuffers.clear();
        m_uboBuffers.reserve(imageCount);
        m_descriptorManager = &descManager;
        
        for (size_t i = 0; i < imageCount; ++i) {
            // Allocate descriptor set
            m_descriptorSets[i] = descManager.allocateDescriptorSet(materialSetLayout);
            
            // Create UBO buffer using RAII
            m_uboBuffers.push_back(std::make_unique<Vulkan::VulkanBuffer>(
                device,
                sizeof(UnifiedMaterialUBO),
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VMA_MEMORY_USAGE_CPU_TO_GPU
            ));
            
            // Update UBO data
            void* mappedData;
            vmaMapMemory(device.getAllocator(), m_uboBuffers[i]->getAllocation(), &mappedData);
            std::memcpy(mappedData, &m_ubo, sizeof(UnifiedMaterialUBO));
            vmaUnmapMemory(device.getAllocator(), m_uboBuffers[i]->getAllocation());
            
            // Setup descriptor writes
            std::vector<VkWriteDescriptorSet> writes;
            std::vector<VkDescriptorBufferInfo> bufferInfos;
            std::vector<VkDescriptorImageInfo> imageInfos;
            
            // UBO binding (binding 0)
            bufferInfos.emplace_back();
            auto& uboInfo = bufferInfos.back();
            uboInfo.buffer = m_uboBuffers[i]->getBuffer();
            uboInfo.offset = 0;
            uboInfo.range = sizeof(UnifiedMaterialUBO);
            
            writes.emplace_back();
            auto& uboWrite = writes.back();
            uboWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            uboWrite.dstSet = m_descriptorSets[i];
            uboWrite.dstBinding = 0;
            uboWrite.dstArrayElement = 0;
            uboWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            uboWrite.descriptorCount = 1;
            uboWrite.pBufferInfo = &uboInfo;
            
            // Texture bindings (binding 1+)
            for (uint32_t slotIndex = 0; slotIndex < static_cast<uint32_t>(TextureSlot::Count); ++slotIndex) {
                TextureSlot slot = static_cast<TextureSlot>(slotIndex);
                auto texture = getTexture(slot);
                
                imageInfos.emplace_back();
                auto& imageInfo = imageInfos.back();
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                
                if (texture) {
                    imageInfo.imageView = texture->getImageView();
                    imageInfo.sampler = texture->getSampler();
                } else {
                    imageInfo.imageView = fallbackView;
                    imageInfo.sampler = fallbackSampler;
                }
                
                writes.emplace_back();
                auto& textureWrite = writes.back();
                textureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                textureWrite.dstSet = m_descriptorSets[i];
                textureWrite.dstBinding = 1 + slotIndex; // UBO is binding 0, textures start at 1
                textureWrite.dstArrayElement = 0;
                textureWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                textureWrite.descriptorCount = 1;
                textureWrite.pImageInfo = &imageInfo;
            }
            
            // Update descriptor set
            descManager.updateDescriptorSet(m_descriptorSets[i], writes);
        }
        
        markClean();
    }

    VkDescriptorSet UnifiedMaterialInstance::getDescriptorSet(size_t imageIndex) const {
        if (imageIndex >= m_descriptorSets.size()) {
            AE_ERROR("Invalid descriptor set index: {} (max: {})", imageIndex, m_descriptorSets.size());
            return VK_NULL_HANDLE;
        }
        return m_descriptorSets[imageIndex];
    }

    void UnifiedMaterialInstance::destroyDescriptorSets() {
        // Clean up UBO buffers (automatic with unique_ptr)
        m_uboBuffers.clear();
        
        // Return allocated descriptor sets to the manager to prevent memory leaks
        if (m_descriptorManager && !m_descriptorSets.empty()) {
            m_descriptorManager->freeDescriptorSets(m_descriptorSets);
        }
        
        m_descriptorSets.clear();
        m_descriptorManager = nullptr;
    }

    void UnifiedMaterialInstance::applyUBOToGPU(Vulkan::VulkanDevice& device, size_t imageIndex) {
        if (!isDirty() || imageIndex >= m_uboBuffers.size()) {
            return;
        }
        
        // Apply template if present
        applyTemplate();
        
        // Update GPU buffer
        void* mappedData;
        vmaMapMemory(device.getAllocator(), m_uboBuffers[imageIndex]->getAllocation(), &mappedData);
        std::memcpy(mappedData, &m_ubo, sizeof(UnifiedMaterialUBO));
        vmaUnmapMemory(device.getAllocator(), m_uboBuffers[imageIndex]->getAllocation());
        
        markClean();
    }

    // === Factory Methods ===

    UnifiedMaterialInstance UnifiedMaterialInstance::createDefault() {
        UnifiedMaterialInstance material;
        material.setBaseColor(glm::vec4(0.8f, 0.8f, 0.8f, 1.0f));
        material.setMetallic(0.0f);
        material.setRoughness(0.5f);
        material.setFeatureFlag(FeatureFlags::CastShadows, true);
        material.setFeatureFlag(FeatureFlags::ReceiveShadows, true);
        return material;
    }

    UnifiedMaterialInstance UnifiedMaterialInstance::createMetal(const glm::vec3& baseColor, float roughness) {
        UnifiedMaterialInstance material;
        material.setBaseColor(glm::vec4(baseColor, 1.0f));
        material.setMetallic(1.0f);
        material.setRoughness(roughness);
        material.setFeatureFlag(FeatureFlags::CastShadows, true);
        material.setFeatureFlag(FeatureFlags::ReceiveShadows, true);
        return material;
    }

    UnifiedMaterialInstance UnifiedMaterialInstance::createDielectric(const glm::vec3& baseColor, float roughness) {
        UnifiedMaterialInstance material;
        material.setBaseColor(glm::vec4(baseColor, 1.0f));
        material.setMetallic(0.0f);
        material.setRoughness(roughness);
        material.setFeatureFlag(FeatureFlags::CastShadows, true);
        material.setFeatureFlag(FeatureFlags::ReceiveShadows, true);
        return material;
    }

    UnifiedMaterialInstance UnifiedMaterialInstance::createPlastic(const glm::vec3& baseColor, float roughness) {
        UnifiedMaterialInstance material;
        material.setBaseColor(glm::vec4(baseColor, 1.0f));
        material.setMetallic(0.0f);
        material.setRoughness(roughness);
        material.setSpecular(glm::vec3(0.04f), 1.0f); // Typical plastic specular
        material.setFeatureFlag(FeatureFlags::CastShadows, true);
        material.setFeatureFlag(FeatureFlags::ReceiveShadows, true);
        return material;
    }

    UnifiedMaterialInstance UnifiedMaterialInstance::createGlass(const glm::vec3& baseColor, float transmission) {
        UnifiedMaterialInstance material;
        material.setBaseColor(glm::vec4(baseColor, 1.0f));
        material.setMetallic(0.0f);
        material.setRoughness(0.0f);
        material.setTransmission(transmission, 1.5f, 0.01f); // Glass IOR
        material.setTransparent();
        material.setFeatureFlag(FeatureFlags::CastShadows, false);
        material.setFeatureFlag(FeatureFlags::ReceiveShadows, true);
        return material;
    }

    UnifiedMaterialInstance UnifiedMaterialInstance::createEmissive(const glm::vec3& emissiveColor, float intensity) {
        UnifiedMaterialInstance material;
        material.setBaseColor(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)); // Black base
        material.setMetallic(0.0f);
        material.setRoughness(1.0f);
        material.setEmissive(emissiveColor, intensity);
        material.setFeatureFlag(FeatureFlags::CastShadows, false);
        material.setFeatureFlag(FeatureFlags::ReceiveShadows, false);
        return material;
    }

    // === MaterialFactory Implementation ===

    UnifiedMaterialInstance MaterialFactory::createFromTemplate(const std::string& templateName) {
        auto it = s_templates.find(templateName);
        if (it != s_templates.end()) {
            return UnifiedMaterialInstance(it->second);
        }
        AE_WARN("Material template not found: {}", templateName);
        return UnifiedMaterialInstance::createDefault();
    }

    void MaterialFactory::registerTemplate(const std::string& name, const MaterialTemplate& materialTemplate) {
        s_templates[name] = materialTemplate;
        AE_INFO("Registered material template: {}", name);
    }

} // namespace AstralEngine
