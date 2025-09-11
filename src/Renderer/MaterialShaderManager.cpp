#include "MaterialShaderManager.h"
#include "UnifiedMaterial.h"
#include "Shader.h"
#include "Vulkan/VulkanDevice.h"
#include "Core/MemoryManager.h"
#include "Core/PerformanceMonitor.h"
#include <sstream>
#include <iostream>
#include <filesystem>

namespace AstralEngine {

    const std::vector<std::string> MaterialShaderManager::s_commonDefines = {
        "MAX_LIGHTS 32",
        "MAX_CASCADES 4",
        "MAX_BONE_INFLUENCES 4"
    };

    MaterialShaderManager::MaterialShaderManager(Vulkan::VulkanDevice& device)
        : m_device(device) {
        // No runtime precompilation - shaders loaded on-demand from offline SPIR-V
    }

    std::shared_ptr<Shader> MaterialShaderManager::getShader(const UnifiedMaterialInstance& material) {
        std::string variant = material.getShaderVariant();
        return getShader(variant);
    }

    std::shared_ptr<Shader> MaterialShaderManager::getShader(const std::string& variant) {
        auto it = m_shaderCache.find(variant);
        if (it != m_shaderCache.end()) {
            return it->second;
        }

        // Load shader variant from offline SPIR-V
        auto shader = createShaderFromVariant(variant);
        if (shader) {
            m_shaderCache[variant] = shader;
        }
        
        return shader;
    }

    // Precompilation removed - offline build provides SPIR-V directly

    // Hot-reload removed - offline build only

    void MaterialShaderManager::printStatistics() const {
        std::cout << "=== MaterialShaderManager Statistics ===" << std::endl;
        std::cout << "Loaded SPIR-V variants: " << m_shaderCache.size() << std::endl;
        std::cout << "Mode: Offline build (no runtime compilation)" << std::endl;
        
        if (!m_shaderCache.empty()) {
            std::cout << "Cached variants:" << std::endl;
            for (const auto& [variant, shader] : m_shaderCache) {
                std::cout << "  - \"" << (variant.empty() ? "default" : variant) << "\"" << std::endl;
            }
        }
        std::cout << "=========================================" << std::endl;
    }

    std::string MaterialShaderManager::generateShaderDefines(const UnifiedMaterialInstance& material) const {
        std::stringstream defines;
        
        // Add common defines
        for (const auto& define : s_commonDefines) {
            defines << "#define " << define << "\n";
        }

        // Texture defines using UnifiedMaterialInstance API
        if (material.hasTextureFlag(TextureFlags::HasBaseColor))
            defines << "#define HAS_BASE_COLOR_TEXTURE\n";
        if (material.hasTextureFlag(TextureFlags::HasMetallicRoughness))
            defines << "#define HAS_METALLIC_ROUGHNESS_TEXTURE\n";
        if (material.hasTextureFlag(TextureFlags::HasNormal))
            defines << "#define HAS_NORMAL_TEXTURE\n";
        if (material.hasTextureFlag(TextureFlags::HasOcclusion))
            defines << "#define HAS_OCCLUSION_TEXTURE\n";
        if (material.hasTextureFlag(TextureFlags::HasEmissive))
            defines << "#define HAS_EMISSIVE_TEXTURE\n";

        // Advanced texture defines
        if (material.hasTextureFlag(TextureFlags::HasAnisotropy))
            defines << "#define HAS_ANISOTROPY_TEXTURE\n";
        if (material.hasTextureFlag(TextureFlags::HasClearcoat))
            defines << "#define HAS_CLEARCOAT_TEXTURE\n";
        if (material.hasTextureFlag(TextureFlags::HasClearcoatNormal))
            defines << "#define HAS_CLEARCOAT_NORMAL_TEXTURE\n";
        if (material.hasTextureFlag(TextureFlags::HasClearcoatRoughness))
            defines << "#define HAS_CLEARCOAT_ROUGHNESS_TEXTURE\n";
        if (material.hasTextureFlag(TextureFlags::HasSheen))
            defines << "#define HAS_SHEEN_TEXTURE\n";
        if (material.hasTextureFlag(TextureFlags::HasSheenColor))
            defines << "#define HAS_SHEEN_COLOR_TEXTURE\n";
        if (material.hasTextureFlag(TextureFlags::HasSheenRoughness))
            defines << "#define HAS_SHEEN_ROUGHNESS_TEXTURE\n";
        if (material.hasTextureFlag(TextureFlags::HasTransmission))
            defines << "#define HAS_TRANSMISSION_TEXTURE\n";
        if (material.hasTextureFlag(TextureFlags::HasVolume))
            defines << "#define HAS_VOLUME_TEXTURE\n";
        if (material.hasTextureFlag(TextureFlags::HasSpecular))
            defines << "#define HAS_SPECULAR_TEXTURE\n";
        if (material.hasTextureFlag(TextureFlags::HasSpecularColor))
            defines << "#define HAS_SPECULAR_COLOR_TEXTURE\n";

        // Material feature defines
        if (material.hasFeatureFlag(FeatureFlags::IsUnlit))
            defines << "#define IS_UNLIT\n";
        if (material.hasFeatureFlag(FeatureFlags::IsDoubleSided))
            defines << "#define IS_DOUBLE_SIDED\n";
        if (material.hasFeatureFlag(FeatureFlags::UseVertexColors))
            defines << "#define USE_VERTEX_COLORS\n";
        if (material.hasFeatureFlag(FeatureFlags::UseSecondaryUV))
            defines << "#define USE_SECONDARY_UV\n";

        // Alpha blending defines
        if (material.hasRenderingFlag(RenderingFlags::AlphaTest))
            defines << "#define ALPHA_TEST\n";
        if (material.hasRenderingFlag(RenderingFlags::AlphaBlend))
            defines << "#define ALPHA_BLEND\n";

        // Lighting defines
        if (material.hasFeatureFlag(FeatureFlags::ReceiveShadows))
            defines << "#define RECEIVE_SHADOWS\n";
        if (material.hasFeatureFlag(FeatureFlags::CastShadows))
            defines << "#define CAST_SHADOWS\n";

        // Advanced PBR features
        if (material.hasFeatureFlag(FeatureFlags::UseAnisotropy))
            defines << "#define USE_ANISOTROPY\n";
        if (material.hasFeatureFlag(FeatureFlags::UseClearcoat))
            defines << "#define USE_CLEARCOAT\n";
        if (material.hasFeatureFlag(FeatureFlags::UseSheen))
            defines << "#define USE_SHEEN\n";
        if (material.hasFeatureFlag(FeatureFlags::UseTransmission))
            defines << "#define USE_TRANSMISSION\n";
        if (material.hasFeatureFlag(FeatureFlags::UseVolume))
            defines << "#define USE_VOLUME\n";
        if (material.hasFeatureFlag(FeatureFlags::UseSpecular))
            defines << "#define USE_SPECULAR\n";

        return defines.str();
    }

    std::shared_ptr<Shader> MaterialShaderManager::createShaderFromVariant(const std::string& variant) {
        try {
            // Determine which fragment shader base to use based on variant
            std::string fragmentShaderBase = pickFragmentShaderBaseFromVariant(variant);
            
            // Always use unified_pbr.vert for vertex shader
            auto shader = std::make_shared<Shader>(m_device, "unified_pbr.vert", fragmentShaderBase);
            
            std::cout << "Loaded shader variant '" << variant << "' using " << fragmentShaderBase << std::endl;
            return shader;
            
        } catch (const std::exception& e) {
            std::cerr << "Failed to load shader variant '" << variant 
                     << "': " << e.what() << std::endl;
            throw;
        }
    }
    
    std::string MaterialShaderManager::pickFragmentShaderBaseFromVariant(const std::string& variant) const {
        // Priority: IS_UNLIT > HAS_BASECOLOR_MAP > default
        // Note: generateShaderDefines creates "UNLIT" for IS_UNLIT and "HAS_BASECOLOR_MAP" for BaseColor texture
        if (variant.find("UNLIT") != std::string::npos) {
            return "unified_pbr_IS_UNLIT.frag";
        } else if (variant.find("HAS_BASECOLOR_MAP") != std::string::npos) {
            return "unified_pbr_HAS_BASECOLOR_MAP.frag";
        } else {
            return "unified_pbr.frag";
        }
    }

} // namespace AstralEngine
