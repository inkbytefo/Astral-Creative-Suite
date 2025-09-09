#pragma once

#include <glm/glm.hpp>
#include <limits>
#include <array>
#include <cstdint>

namespace AstralEngine {

    // === Material Flags ===
    
    // Texture presence flags (first 16 bits)
    enum class TextureFlags : uint32_t {
        None = 0,
        HasBaseColor            = 1 << 0,
        HasMetallicRoughness    = 1 << 1,
        HasNormal               = 1 << 2,
        HasOcclusion            = 1 << 3,
        HasEmissive             = 1 << 4,
        HasAnisotropy           = 1 << 5,
        HasClearcoat            = 1 << 6,
        HasClearcoatNormal      = 1 << 7,
        HasClearcoatRoughness   = 1 << 8,
        HasSheen                = 1 << 9,
        HasSheenColor           = 1 << 10,
        HasSheenRoughness       = 1 << 11,
        HasTransmission         = 1 << 12,
        HasVolume               = 1 << 13,
        HasSpecular             = 1 << 14,
        HasSpecularColor        = 1 << 15
    };

    // Feature flags (second 16 bits)
    enum class FeatureFlags : uint32_t {
        None = 0,
        UseAnisotropy          = 1 << 0,
        UseClearcoat           = 1 << 1,
        UseSheen               = 1 << 2,
        UseTransmission        = 1 << 3,
        UseVolume              = 1 << 4,
        UseSpecular            = 1 << 5,
        UseVertexColors        = 1 << 6,
        UseSecondaryUV         = 1 << 7,
        IsUnlit                = 1 << 8,
        IsDoubleSided          = 1 << 9,
        CastShadows           = 1 << 10,
        ReceiveShadows        = 1 << 11,
        Reserved1             = 1 << 12,
        Reserved2             = 1 << 13,
        Reserved3             = 1 << 14,
        Reserved4             = 1 << 15
    };

    // Alpha modes (third component)
    enum class AlphaMode : uint32_t {
        Opaque = 0,
        Mask = 1,
        Blend = 2
    };

    // Rendering flags (fourth component)
    enum class RenderingFlags : uint32_t {
        None = 0,
        AlphaTest              = 1 << 0,
        AlphaBlend             = 1 << 1,
        BackfaceCulling        = 1 << 2,
        FrontfaceCulling       = 1 << 3,
        DoubleSided            = 1 << 4,
        Reserved1              = 1 << 5,
        Reserved2              = 1 << 6,
        Reserved3              = 1 << 7
    };

    // Forward declaration for UBO
    struct UnifiedMaterialUBO;

    // Flag check helpers
    inline bool hasTextureFlag(const UnifiedMaterialUBO& material, TextureFlags flag);
    inline bool hasFeatureFlag(const UnifiedMaterialUBO& material, FeatureFlags flag);
    inline bool hasRenderingFlag(const UnifiedMaterialUBO& material, RenderingFlags flag);
    inline AlphaMode getAlphaMode(const UnifiedMaterialUBO& material);

    // Flag setters
    inline void setTextureFlag(UnifiedMaterialUBO& material, TextureFlags flag, bool enabled);
    inline void setFeatureFlag(UnifiedMaterialUBO& material, FeatureFlags flag, bool enabled);
    inline void setAlphaMode(UnifiedMaterialUBO& material, AlphaMode mode);
    inline void setRenderingFlag(UnifiedMaterialUBO& material, RenderingFlags flag, bool enabled);

    /**
     * Unified Material UBO Structure
     * Combines the best of MaterialUBO and PBRMaterialUBO into a single,
     * comprehensive material system that supports both legacy and advanced PBR workflows.
     * 
     * Memory layout is carefully designed for std140 compliance and GPU efficiency.
     * Total size: 320 bytes (20 * vec4 = 20 * 16 bytes)
     */
    struct alignas(16) UnifiedMaterialUBO {
        // === Base PBR Properties (64 bytes) ===
        alignas(16) glm::vec4 baseColor;                    // RGBA base color + opacity
        alignas(16) glm::vec3 emissiveColor; float metallic; // RGB emissive + metallic factor
        alignas(16) glm::vec3 specularColor; float roughness; // RGB specular + roughness factor
        alignas(16) glm::vec4 normalOcclusionParams;         // normalScale, occlusionStrength, alphaCutoff, specularFactor

        // === Advanced PBR Properties (80 bytes) ===
        alignas(16) glm::vec4 anisotropyParams;              // anisotropy, anisotropyRotation, clearcoat, clearcoatRoughness
        alignas(16) glm::vec4 sheenParams;                   // sheen, sheenRoughness, padding, padding
        alignas(16) glm::vec3 sheenColor; float clearcoatNormalScale;
        alignas(16) glm::vec4 transmissionParams;            // transmission, ior, thickness, volume
        alignas(16) glm::vec4 volumeParams;                  // volumeThickness, volumeAttenuationDistance, padding, padding

        // === Attenuation Colors (32 bytes) ===
        alignas(16) glm::vec3 attenuationColor; float attenuationDistance;
        alignas(16) glm::vec3 volumeAttenuationColor; float volumeAttenuationDistance;

        // === Texture Coordinate Transforms (64 bytes) ===
        alignas(16) glm::vec4 baseColorTilingOffset;         // tiling.xy, offset.xy
        alignas(16) glm::vec4 normalTilingOffset;            // tiling.xy, offset.xy  
        alignas(16) glm::vec4 metallicRoughnessTilingOffset; // tiling.xy, offset.xy
        alignas(16) glm::vec4 emissiveTilingOffset;          // tiling.xy, offset.xy

        // === Texture Indices (64 bytes) ===
        alignas(16) glm::uvec4 textureIndices1;             // baseColor, metallicRoughness, normal, occlusion
        alignas(16) glm::uvec4 textureIndices2;             // emissive, anisotropy, clearcoat, clearcoatNormal
        alignas(16) glm::uvec4 textureIndices3;             // clearcoatRoughness, sheen, sheenColor, sheenRoughness
        alignas(16) glm::uvec4 textureIndices4;             // transmission, volume, specular, specularColor

        // === Material Flags and Settings (16 bytes) ===
        alignas(16) glm::uvec4 materialFlags;               // textureFlags, featureFlags, alphaMode, renderingFlags

        // === Constructors ===
        UnifiedMaterialUBO() {
            // Initialize base PBR properties
            baseColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            emissiveColor = glm::vec3(0.0f);
            metallic = 0.0f;
            specularColor = glm::vec3(1.0f);
            roughness = 0.5f;
            normalOcclusionParams = glm::vec4(1.0f, 1.0f, 0.5f, 1.0f); // normalScale, occlusionStrength, alphaCutoff, specularFactor

            // Initialize advanced PBR properties
            anisotropyParams = glm::vec4(0.0f);              // anisotropy, anisotropyRotation, clearcoat, clearcoatRoughness
            sheenParams = glm::vec4(0.0f);                   // sheen, sheenRoughness, padding, padding
            sheenColor = glm::vec3(0.0f);
            clearcoatNormalScale = 1.0f;
            transmissionParams = glm::vec4(0.0f, 1.5f, 0.0f, 0.0f); // transmission, ior, thickness, volume
            volumeParams = glm::vec4(0.0f, std::numeric_limits<float>::max(), 0.0f, 0.0f); // volumeThickness, volumeAttenuationDistance, padding, padding

            // Initialize attenuation colors
            attenuationColor = glm::vec3(1.0f);
            attenuationDistance = std::numeric_limits<float>::max();
            volumeAttenuationColor = glm::vec3(1.0f);
            volumeAttenuationDistance = std::numeric_limits<float>::max();

            // Initialize texture coordinate transforms with default (no transform)
            baseColorTilingOffset = glm::vec4(1.0f, 1.0f, 0.0f, 0.0f);
            normalTilingOffset = glm::vec4(1.0f, 1.0f, 0.0f, 0.0f);
            metallicRoughnessTilingOffset = glm::vec4(1.0f, 1.0f, 0.0f, 0.0f);
            emissiveTilingOffset = glm::vec4(1.0f, 1.0f, 0.0f, 0.0f);

            // Initialize texture indices to invalid
            const uint32_t invalidTexture = ~0u;
            textureIndices1 = glm::uvec4(invalidTexture);
            textureIndices2 = glm::uvec4(invalidTexture);
            textureIndices3 = glm::uvec4(invalidTexture);
            textureIndices4 = glm::uvec4(invalidTexture);

            // Initialize flags
            materialFlags = glm::uvec4(0);
        }

        // === Convenience Accessors ===
        
        // Texture coordinate accessors
        glm::vec2& getBaseColorTiling() { return reinterpret_cast<glm::vec2&>(baseColorTilingOffset.x); }
        glm::vec2& getBaseColorOffset() { return reinterpret_cast<glm::vec2&>(baseColorTilingOffset.z); }
        glm::vec2& getNormalTiling() { return reinterpret_cast<glm::vec2&>(normalTilingOffset.x); }
        glm::vec2& getNormalOffset() { return reinterpret_cast<glm::vec2&>(normalTilingOffset.z); }
        glm::vec2& getMetallicRoughnessTiling() { return reinterpret_cast<glm::vec2&>(metallicRoughnessTilingOffset.x); }
        glm::vec2& getMetallicRoughnessOffset() { return reinterpret_cast<glm::vec2&>(metallicRoughnessTilingOffset.z); }
        glm::vec2& getEmissiveTiling() { return reinterpret_cast<glm::vec2&>(emissiveTilingOffset.x); }
        glm::vec2& getEmissiveOffset() { return reinterpret_cast<glm::vec2&>(emissiveTilingOffset.z); }

        // Parameter accessors
        float& getNormalScale() { return normalOcclusionParams.x; }
        float& getOcclusionStrength() { return normalOcclusionParams.y; }
        float& getAlphaCutoff() { return normalOcclusionParams.z; }
        float& getSpecularFactor() { return normalOcclusionParams.w; }

        float& getAnisotropy() { return anisotropyParams.x; }
        float& getAnisotropyRotation() { return anisotropyParams.y; }
        float& getClearcoat() { return anisotropyParams.z; }
        float& getClearcoatRoughness() { return anisotropyParams.w; }

        float& getSheen() { return sheenParams.x; }
        float& getSheenRoughness() { return sheenParams.y; }

        float& getTransmission() { return transmissionParams.x; }
        float& getIor() { return transmissionParams.y; }
        float& getThickness() { return transmissionParams.z; }
        float& getVolume() { return transmissionParams.w; }

        float& getVolumeThickness() { return volumeParams.x; }
        float& getVolumeAttenuationDistance() { return volumeParams.y; }

        // Const versions
        const glm::vec2& getBaseColorTiling() const { return reinterpret_cast<const glm::vec2&>(baseColorTilingOffset.x); }
        const glm::vec2& getBaseColorOffset() const { return reinterpret_cast<const glm::vec2&>(baseColorTilingOffset.z); }
        const glm::vec2& getNormalTiling() const { return reinterpret_cast<const glm::vec2&>(normalTilingOffset.x); }
        const glm::vec2& getNormalOffset() const { return reinterpret_cast<const glm::vec2&>(normalTilingOffset.z); }
        const glm::vec2& getMetallicRoughnessTiling() const { return reinterpret_cast<const glm::vec2&>(metallicRoughnessTilingOffset.x); }
        const glm::vec2& getMetallicRoughnessOffset() const { return reinterpret_cast<const glm::vec2&>(metallicRoughnessTilingOffset.z); }
        const glm::vec2& getEmissiveTiling() const { return reinterpret_cast<const glm::vec2&>(emissiveTilingOffset.x); }
        const glm::vec2& getEmissiveOffset() const { return reinterpret_cast<const glm::vec2&>(emissiveTilingOffset.z); }

        float getNormalScale() const { return normalOcclusionParams.x; }
        float getOcclusionStrength() const { return normalOcclusionParams.y; }
        float getAlphaCutoff() const { return normalOcclusionParams.z; }
        float getSpecularFactor() const { return normalOcclusionParams.w; }

        float getAnisotropy() const { return anisotropyParams.x; }
        float getAnisotropyRotation() const { return anisotropyParams.y; }
        float getClearcoat() const { return anisotropyParams.z; }
        float getClearcoatRoughness() const { return anisotropyParams.w; }

        float getSheen() const { return sheenParams.x; }
        float getSheenRoughness() const { return sheenParams.y; }

        float getTransmission() const { return transmissionParams.x; }
        float getIor() const { return transmissionParams.y; }
        float getThickness() const { return transmissionParams.z; }
        float getVolume() const { return transmissionParams.w; }

        float getVolumeThickness() const { return volumeParams.x; }
        float getVolumeAttenuationDistance() const { return volumeParams.y; }
        
        // === Flag Helper Methods ===
        
        // Check if flag is set
        bool hasTextureFlag(TextureFlags flag) const {
            return ::AstralEngine::hasTextureFlag(*this, flag);
        }
        
        bool hasFeatureFlag(FeatureFlags flag) const {
            return ::AstralEngine::hasFeatureFlag(*this, flag);
        }
        
        bool hasRenderingFlag(RenderingFlags flag) const {
            return ::AstralEngine::hasRenderingFlag(*this, flag);
        }
        
        // Set flags
        void setTextureFlag(TextureFlags flag, bool enabled) {
            ::AstralEngine::setTextureFlag(*this, flag, enabled);
        }
        
        void setFeatureFlag(FeatureFlags flag, bool enabled) {
            ::AstralEngine::setFeatureFlag(*this, flag, enabled);
        }
        
        void setRenderingFlag(RenderingFlags flag, bool enabled) {
            ::AstralEngine::setRenderingFlag(*this, flag, enabled);
        }
    };

    // Compile-time size validation
    static_assert(sizeof(UnifiedMaterialUBO) == 320, "UnifiedMaterialUBO must be exactly 320 bytes for std140 layout");
    static_assert(alignof(UnifiedMaterialUBO) == 16, "UnifiedMaterialUBO must be 16-byte aligned");

    // Flag operators
    inline TextureFlags operator|(TextureFlags a, TextureFlags b) {
        return static_cast<TextureFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    inline TextureFlags operator&(TextureFlags a, TextureFlags b) {
        return static_cast<TextureFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }

    inline FeatureFlags operator|(FeatureFlags a, FeatureFlags b) {
        return static_cast<FeatureFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    inline FeatureFlags operator&(FeatureFlags a, FeatureFlags b) {
        return static_cast<FeatureFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }

    inline RenderingFlags operator|(RenderingFlags a, RenderingFlags b) {
        return static_cast<RenderingFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    inline RenderingFlags operator&(RenderingFlags a, RenderingFlags b) {
        return static_cast<RenderingFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }

    // Flag check helpers
    inline bool hasTextureFlag(const UnifiedMaterialUBO& material, TextureFlags flag) {
        return (static_cast<TextureFlags>(material.materialFlags.x) & flag) == flag;
    }

    inline bool hasFeatureFlag(const UnifiedMaterialUBO& material, FeatureFlags flag) {
        return (static_cast<FeatureFlags>(material.materialFlags.y) & flag) == flag;
    }

    inline AlphaMode getAlphaMode(const UnifiedMaterialUBO& material) {
        return static_cast<AlphaMode>(material.materialFlags.z);
    }

    inline bool hasRenderingFlag(const UnifiedMaterialUBO& material, RenderingFlags flag) {
        return (static_cast<RenderingFlags>(material.materialFlags.w) & flag) == flag;
    }

    // Flag setters
    inline void setTextureFlag(UnifiedMaterialUBO& material, TextureFlags flag, bool enabled) {
        uint32_t flags = material.materialFlags.x;
        if (enabled) {
            flags |= static_cast<uint32_t>(flag);
        } else {
            flags &= ~static_cast<uint32_t>(flag);
        }
        material.materialFlags.x = flags;
    }

    inline void setFeatureFlag(UnifiedMaterialUBO& material, FeatureFlags flag, bool enabled) {
        uint32_t flags = material.materialFlags.y;
        if (enabled) {
            flags |= static_cast<uint32_t>(flag);
        } else {
            flags &= ~static_cast<uint32_t>(flag);
        }
        material.materialFlags.y = flags;
    }

    inline void setAlphaMode(UnifiedMaterialUBO& material, AlphaMode mode) {
        material.materialFlags.z = static_cast<uint32_t>(mode);
    }

    inline void setRenderingFlag(UnifiedMaterialUBO& material, RenderingFlags flag, bool enabled) {
        uint32_t flags = material.materialFlags.w;
        if (enabled) {
            flags |= static_cast<uint32_t>(flag);
        } else {
            flags &= ~static_cast<uint32_t>(flag);
        }
        material.materialFlags.w = flags;
    }

    // === Material Template System ===
    
    /**
     * Material template for inheritance and parameterization.
     * Allows creating material "classes" that can be instantiated with variations.
     */
    struct MaterialTemplate {
        UnifiedMaterialUBO defaultParameters;
        uint64_t lockedParametersMask{0}; // Bitmask of locked parameters (64-bit for future expansion)
        
        // Parameter lock masks (can be extended as needed)
        enum ParameterLock : uint64_t {
            LOCK_BASE_COLOR         = 1ULL << 0,
            LOCK_METALLIC           = 1ULL << 1, 
            LOCK_ROUGHNESS          = 1ULL << 2,
            LOCK_NORMAL_SCALE       = 1ULL << 3,
            LOCK_EMISSIVE           = 1ULL << 4,
            LOCK_TEXTURE_TILING     = 1ULL << 5,
            LOCK_ANISOTROPY         = 1ULL << 6,
            LOCK_CLEARCOAT          = 1ULL << 7,
            LOCK_SHEEN              = 1ULL << 8,
            LOCK_TRANSMISSION       = 1ULL << 9,
            LOCK_TEXTURE_FLAGS      = 1ULL << 10,
            LOCK_FEATURE_FLAGS      = 1ULL << 11,
            LOCK_ALPHA_MODE         = 1ULL << 12,
            LOCK_RENDERING_FLAGS    = 1ULL << 13
        };
        
        void lockParameter(ParameterLock mask, bool locked = true) {
            if (locked) {
                lockedParametersMask |= mask;
            } else {
                lockedParametersMask &= ~mask;
            }
        }
        
        bool isParameterLocked(ParameterLock mask) const {
            return (lockedParametersMask & mask) != 0;
        }
        
        // Apply template to instance parameters, respecting locks
        UnifiedMaterialUBO applyTemplate(const UnifiedMaterialUBO& instanceParams) const;
    };

    // === Texture Slot Management ===
    
    enum class TextureSlot : uint32_t {
        BaseColor = 0,
        MetallicRoughness = 1,
        Normal = 2,
        Occlusion = 3,
        Emissive = 4,
        Anisotropy = 5,
        Clearcoat = 6,
        ClearcoatNormal = 7,
        ClearcoatRoughness = 8,
        Sheen = 9,
        SheenColor = 10,
        SheenRoughness = 11,
        Transmission = 12,
        Volume = 13,
        Specular = 14,
        SpecularColor = 15,
        Count = 16
    };

    // Texture format preferences
    enum class TextureFormat {
        SRGB,      // For color textures (base color, emissive, sheen color)
        LINEAR,    // For data textures (normal, metallic-roughness, etc.)
        AUTO       // Let the system decide based on texture slot
    };

    // Get preferred format for texture slot
    constexpr TextureFormat getPreferredFormat(TextureSlot slot) {
        switch (slot) {
            case TextureSlot::BaseColor:
            case TextureSlot::Emissive:
            case TextureSlot::SheenColor:
                return TextureFormat::SRGB;
            case TextureSlot::Normal:
            case TextureSlot::MetallicRoughness:
            case TextureSlot::Occlusion:
            case TextureSlot::Anisotropy:
            case TextureSlot::Clearcoat:
            case TextureSlot::ClearcoatNormal:
            case TextureSlot::ClearcoatRoughness:
            case TextureSlot::Sheen:
            case TextureSlot::SheenRoughness:
            case TextureSlot::Transmission:
            case TextureSlot::Volume:
            case TextureSlot::Specular:
            case TextureSlot::SpecularColor:
                return TextureFormat::LINEAR;
            default:
                return TextureFormat::LINEAR;
        }
    }

    // Helper to get texture index within the appropriate indices vector
    inline void setTextureIndex(UnifiedMaterialUBO& material, TextureSlot slot, uint32_t index) {
        uint32_t slotIndex = static_cast<uint32_t>(slot);
        if (slotIndex < 4) {
            material.textureIndices1[slotIndex] = index;
        } else if (slotIndex < 8) {
            material.textureIndices2[slotIndex - 4] = index;
        } else if (slotIndex < 12) {
            material.textureIndices3[slotIndex - 8] = index;
        } else if (slotIndex < 16) {
            material.textureIndices4[slotIndex - 12] = index;
        }
    }

    inline uint32_t getTextureIndex(const UnifiedMaterialUBO& material, TextureSlot slot) {
        uint32_t slotIndex = static_cast<uint32_t>(slot);
        if (slotIndex < 4) {
            return material.textureIndices1[slotIndex];
        } else if (slotIndex < 8) {
            return material.textureIndices2[slotIndex - 4];
        } else if (slotIndex < 12) {
            return material.textureIndices3[slotIndex - 8];
        } else if (slotIndex < 16) {
            return material.textureIndices4[slotIndex - 12];
        }
        return ~0u; // Invalid texture index
    }

} // namespace AstralEngine
