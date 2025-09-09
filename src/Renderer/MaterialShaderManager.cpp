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
        precompileCommonVariants();
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

        // Compile new shader variant
        auto shader = compileShader(variant);
        if (shader) {
            m_shaderCache[variant] = shader;
        }
        
        return shader;
    }

    void MaterialShaderManager::precompileCommonVariants() {
        PERF_TIMER("MaterialShaderManager::precompileCommonVariants");
        STACK_SCOPE(); // Use stack allocator for shader compilation
        
        // Common shader variants that are likely to be used
        std::vector<std::string> commonVariants = {
            // Basic variants
            "",
            "HAS_BASE_COLOR_TEXTURE",
            "HAS_METALLIC_ROUGHNESS_TEXTURE",
            "HAS_NORMAL_TEXTURE",
            
            // Combined variants
            "HAS_BASE_COLOR_TEXTURE HAS_METALLIC_ROUGHNESS_TEXTURE",
            "HAS_BASE_COLOR_TEXTURE HAS_NORMAL_TEXTURE",
            "HAS_BASE_COLOR_TEXTURE HAS_METALLIC_ROUGHNESS_TEXTURE HAS_NORMAL_TEXTURE",
            "HAS_BASE_COLOR_TEXTURE HAS_METALLIC_ROUGHNESS_TEXTURE HAS_NORMAL_TEXTURE HAS_OCCLUSION_TEXTURE",
            
            // Transparency variants
            "ALPHA_TEST",
            "ALPHA_BLEND",
            "HAS_BASE_COLOR_TEXTURE ALPHA_TEST",
            "HAS_BASE_COLOR_TEXTURE ALPHA_BLEND",
            
            // Unlit variant
            "IS_UNLIT",
            "IS_UNLIT HAS_BASE_COLOR_TEXTURE",
            
            // Advanced PBR features
            "USE_CLEARCOAT",
            "USE_TRANSMISSION",
            "USE_SHEEN",
            "USE_ANISOTROPY"
        };

        std::cout << "Precompiling " << commonVariants.size() << " common shader variants..." << std::endl;
        
        for (const auto& variant : commonVariants) {
            try {
                getShader(variant);
            } catch (const std::exception& e) {
                std::cerr << "Failed to precompile shader variant '" << variant 
                         << "': " << e.what() << std::endl;
            }
        }

        std::cout << "Successfully compiled " << m_shaderCache.size() 
                 << " shader variants." << std::endl;
    }

    void MaterialShaderManager::reloadShaders() {
        if (!m_hotReloadEnabled) {
            return;
        }

        std::cout << "Reloading all shader variants..." << std::endl;
        
        // Save current variants
        std::vector<std::string> variants;
        for (const auto& [variant, shader] : m_shaderCache) {
            variants.push_back(variant);
        }

        // Clear cache
        m_shaderCache.clear();

        // Recompile all variants
        for (const auto& variant : variants) {
            try {
                compileShader(variant);
            } catch (const std::exception& e) {
                std::cerr << "Failed to reload shader variant '" << variant 
                         << "': " << e.what() << std::endl;
            }
        }

        std::cout << "Reloaded " << m_shaderCache.size() << " shader variants." << std::endl;
    }

    void MaterialShaderManager::printStatistics() const {
        std::cout << "=== MaterialShaderManager Statistics ===" << std::endl;
        std::cout << "Compiled variants: " << m_shaderCache.size() << std::endl;
        std::cout << "Hot reload enabled: " << (m_hotReloadEnabled ? "Yes" : "No") << std::endl;
        std::cout << "Vertex shader path: " << m_vertexShaderPath << std::endl;
        std::cout << "Fragment shader path: " << m_fragmentShaderPath << std::endl;
        
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

    std::shared_ptr<Shader> MaterialShaderManager::compileShader(const std::string& variant) {
        PERF_TIMER("MaterialShaderManager::compileShader");
        STACK_SCOPE(); // Use stack allocator for shader compilation
        
        try {
            // Debug: print working directory and paths
            std::cout << "Current working directory: " << std::filesystem::current_path() << std::endl;
            std::cout << "Vertex shader path: " << m_vertexShaderPath << std::endl;
            std::cout << "Fragment shader path: " << m_fragmentShaderPath << std::endl;
            
            // Check if files exist
            std::cout << "Vertex shader exists: " << std::filesystem::exists(m_vertexShaderPath + ".spv") << std::endl;
            std::cout << "Fragment shader exists: " << std::filesystem::exists(m_fragmentShaderPath + ".spv") << std::endl;
            
            // Create shader with variant defines
            auto shader = std::make_shared<Shader>(m_device, m_vertexShaderPath, m_fragmentShaderPath);
            
            // Apply variant defines if needed
            if (!variant.empty()) {
                // For now, we assume the shader compilation system handles defines
                // In a real implementation, you would pass the defines to the shader compiler
                std::cout << "Compiling shader variant: " << variant << std::endl;
            }
            
            return shader;
            
        } catch (const std::exception& e) {
            std::cerr << "Failed to compile shader variant '" << variant 
                     << "': " << e.what() << std::endl;
            throw;
        }
    }

} // namespace AstralEngine
