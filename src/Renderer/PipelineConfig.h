#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <functional>
#include <cstring>

namespace AstralEngine {
    
    // Rasterization configuration
    struct RasterizationConfig {
        VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
        VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
        VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; // Standard for most mesh formats
        bool depthClampEnable = false;
        bool rasterizerDiscardEnable = false;
        float lineWidth = 1.0f;
        bool depthBiasEnable = false;
        float depthBiasConstantFactor = 0.0f;
        float depthBiasSlopeFactor = 0.0f;
        
        // Helper methods for common configurations
        static RasterizationConfig createDefault() {
            RasterizationConfig config;
            config.cullMode = VK_CULL_MODE_BACK_BIT;
            config.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
            return config;
        }
        
        static RasterizationConfig createDoubleSided() {
            RasterizationConfig config;
            config.cullMode = VK_CULL_MODE_NONE;
            return config;
        }
        
        static RasterizationConfig createWireframe() {
            RasterizationConfig config;
            config.polygonMode = VK_POLYGON_MODE_LINE;
            config.cullMode = VK_CULL_MODE_NONE;
            return config;
        }
        
        // For engines that generate meshes with clockwise winding (like some CAD software)
        static RasterizationConfig createClockwiseFacing() {
            RasterizationConfig config;
            config.cullMode = VK_CULL_MODE_BACK_BIT;
            config.frontFace = VK_FRONT_FACE_CLOCKWISE;
            return config;
        }
    };
    
    // Depth-stencil configuration
    struct DepthStencilConfig {
        bool depthTestEnable = true;
        bool depthWriteEnable = true;
        VkCompareOp depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        bool depthBoundsTestEnable = false;
        float minDepthBounds = 0.0f;
        float maxDepthBounds = 1.0f;
        bool stencilTestEnable = false;
        VkStencilOpState front = {};
        VkStencilOpState back = {};
        
        static DepthStencilConfig createDefault() {
            DepthStencilConfig config;
            config.depthTestEnable = true;
            config.depthWriteEnable = true;
            config.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
            return config;
        }
        
        static DepthStencilConfig createTransparent() {
            DepthStencilConfig config;
            config.depthTestEnable = true;
            config.depthWriteEnable = false; // Don't write to depth for transparent objects
            config.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
            return config;
        }
        
        static DepthStencilConfig createDisabled() {
            DepthStencilConfig config;
            config.depthTestEnable = false;
            config.depthWriteEnable = false;
            return config;
        }
    };
    
    // Blending configuration
    struct BlendConfig {
        bool blendEnable = false;
        VkBlendFactor srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        VkBlendFactor dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        VkBlendOp colorBlendOp = VK_BLEND_OP_ADD;
        VkBlendFactor srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        VkBlendFactor dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        VkBlendOp alphaBlendOp = VK_BLEND_OP_ADD;
        VkColorComponentFlags colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        
        static BlendConfig createOpaque() {
            BlendConfig config;
            config.blendEnable = false;
            return config;
        }
        
        static BlendConfig createAlphaBlend() {
            BlendConfig config;
            config.blendEnable = true;
            config.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            config.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            config.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            config.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            return config;
        }
        
        static BlendConfig createAdditive() {
            BlendConfig config;
            config.blendEnable = true;
            config.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
            config.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
            return config;
        }
    };
    
    // Complete pipeline configuration
    struct PipelineConfig {
        RasterizationConfig rasterization = RasterizationConfig::createDefault();
        DepthStencilConfig depthStencil = DepthStencilConfig::createDefault();
        BlendConfig blend = BlendConfig::createOpaque();
        VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        bool primitiveRestartEnable = false;
        
        // Multisampling
        VkSampleCountFlagBits rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        bool sampleShadingEnable = false;
        
        // Dynamic states
        std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        
        // Predefined configurations
        static PipelineConfig createDefault() {
            PipelineConfig config;
            return config;
        }
        
        static PipelineConfig createTransparent() {
            PipelineConfig config;
            config.depthStencil = DepthStencilConfig::createTransparent();
            config.blend = BlendConfig::createAlphaBlend();
            return config;
        }
        
        static PipelineConfig createDoubleSided() {
            PipelineConfig config;
            config.rasterization = RasterizationConfig::createDoubleSided();
            return config;
        }
        
        static PipelineConfig createWireframe() {
            PipelineConfig config;
            config.rasterization = RasterizationConfig::createWireframe();
            return config;
        }
    };
    
    // Forward declaration
    struct UnifiedMaterialUBO;
    
    // Utility functions to determine appropriate pipeline config from material data
    PipelineConfig getPipelineConfigFromMaterialFlags(uint32_t materialFlags);
    PipelineConfig getPipelineConfigFromUnifiedMaterial(const UnifiedMaterialUBO& ubo);
    
    // ========== Hash and Equality Utilities ==========
    
    // Simple hash combine utility
    inline void hash_combine(std::size_t& seed, std::size_t value) {
        seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
    }
    
    inline std::size_t hash_bool(bool v) { return v ? 0x9e3779b9 : 0x85ebca6b; }
    
    inline std::size_t hash_span(const std::vector<VkDynamicState>& v) {
        std::size_t seed = 0;
        for (auto s : v) {
            hash_combine(seed, std::hash<uint32_t>{}(static_cast<uint32_t>(s)));
        }
        return seed;
    }
    
    // ========== Equality Operators ==========
    
    inline bool operator==(const RasterizationConfig& a, const RasterizationConfig& b) {
        return a.polygonMode == b.polygonMode &&
               a.cullMode == b.cullMode &&
               a.frontFace == b.frontFace &&
               a.depthClampEnable == b.depthClampEnable &&
               a.rasterizerDiscardEnable == b.rasterizerDiscardEnable &&
               a.lineWidth == b.lineWidth &&
               a.depthBiasEnable == b.depthBiasEnable &&
               a.depthBiasConstantFactor == b.depthBiasConstantFactor &&
               a.depthBiasSlopeFactor == b.depthBiasSlopeFactor;
    }
    
    inline bool operator==(const DepthStencilConfig& a, const DepthStencilConfig& b) {
        return a.depthTestEnable == b.depthTestEnable &&
               a.depthWriteEnable == b.depthWriteEnable &&
               a.depthCompareOp == b.depthCompareOp &&
               a.depthBoundsTestEnable == b.depthBoundsTestEnable &&
               a.minDepthBounds == b.minDepthBounds &&
               a.maxDepthBounds == b.maxDepthBounds &&
               a.stencilTestEnable == b.stencilTestEnable &&
               std::memcmp(&a.front, &b.front, sizeof(VkStencilOpState)) == 0 &&
               std::memcmp(&a.back, &b.back, sizeof(VkStencilOpState)) == 0;
    }
    
    inline bool operator==(const BlendConfig& a, const BlendConfig& b) {
        return a.blendEnable == b.blendEnable &&
               a.srcColorBlendFactor == b.srcColorBlendFactor &&
               a.dstColorBlendFactor == b.dstColorBlendFactor &&
               a.colorBlendOp == b.colorBlendOp &&
               a.srcAlphaBlendFactor == b.srcAlphaBlendFactor &&
               a.dstAlphaBlendFactor == b.dstAlphaBlendFactor &&
               a.alphaBlendOp == b.alphaBlendOp &&
               a.colorWriteMask == b.colorWriteMask;
    }
    
    inline bool operator==(const PipelineConfig& a, const PipelineConfig& b) {
        return a.rasterization == b.rasterization &&
               a.depthStencil == b.depthStencil &&
               a.blend == b.blend &&
               a.topology == b.topology &&
               a.primitiveRestartEnable == b.primitiveRestartEnable &&
               a.rasterizationSamples == b.rasterizationSamples &&
               a.sampleShadingEnable == b.sampleShadingEnable &&
               a.dynamicStates == b.dynamicStates;
    }
    
    // ========== Hash Functions ==========
    
    inline std::size_t hash_value(const RasterizationConfig& v) {
        std::size_t seed = 0;
        hash_combine(seed, std::hash<uint32_t>{}(static_cast<uint32_t>(v.polygonMode)));
        hash_combine(seed, std::hash<uint32_t>{}(static_cast<uint32_t>(v.cullMode)));
        hash_combine(seed, std::hash<uint32_t>{}(static_cast<uint32_t>(v.frontFace)));
        hash_combine(seed, hash_bool(v.depthClampEnable));
        hash_combine(seed, hash_bool(v.rasterizerDiscardEnable));
        hash_combine(seed, std::hash<float>{}(v.lineWidth));
        hash_combine(seed, hash_bool(v.depthBiasEnable));
        hash_combine(seed, std::hash<float>{}(v.depthBiasConstantFactor));
        hash_combine(seed, std::hash<float>{}(v.depthBiasSlopeFactor));
        return seed;
    }
    
    inline std::size_t hash_value(const DepthStencilConfig& v) {
        std::size_t seed = 0;
        hash_combine(seed, hash_bool(v.depthTestEnable));
        hash_combine(seed, hash_bool(v.depthWriteEnable));
        hash_combine(seed, std::hash<uint32_t>{}(static_cast<uint32_t>(v.depthCompareOp)));
        hash_combine(seed, hash_bool(v.depthBoundsTestEnable));
        hash_combine(seed, std::hash<float>{}(v.minDepthBounds));
        hash_combine(seed, std::hash<float>{}(v.maxDepthBounds));
        hash_combine(seed, hash_bool(v.stencilTestEnable));
        // Stencil states: treat as bytes for hashing
        const uint8_t* f = reinterpret_cast<const uint8_t*>(&v.front);
        const uint8_t* b = reinterpret_cast<const uint8_t*>(&v.back);
        for (size_t i = 0; i < sizeof(VkStencilOpState); ++i) hash_combine(seed, f[i]);
        for (size_t i = 0; i < sizeof(VkStencilOpState); ++i) hash_combine(seed, b[i]);
        return seed;
    }
    
    inline std::size_t hash_value(const BlendConfig& v) {
        std::size_t seed = 0;
        hash_combine(seed, hash_bool(v.blendEnable));
        hash_combine(seed, std::hash<uint32_t>{}(static_cast<uint32_t>(v.srcColorBlendFactor)));
        hash_combine(seed, std::hash<uint32_t>{}(static_cast<uint32_t>(v.dstColorBlendFactor)));
        hash_combine(seed, std::hash<uint32_t>{}(static_cast<uint32_t>(v.colorBlendOp)));
        hash_combine(seed, std::hash<uint32_t>{}(static_cast<uint32_t>(v.srcAlphaBlendFactor)));
        hash_combine(seed, std::hash<uint32_t>{}(static_cast<uint32_t>(v.dstAlphaBlendFactor)));
        hash_combine(seed, std::hash<uint32_t>{}(static_cast<uint32_t>(v.alphaBlendOp)));
        hash_combine(seed, std::hash<uint32_t>{}(static_cast<uint32_t>(v.colorWriteMask)));
        return seed;
    }
    
    inline std::size_t hash_value(const PipelineConfig& v) {
        std::size_t seed = 0;
        hash_combine(seed, hash_value(v.rasterization));
        hash_combine(seed, hash_value(v.depthStencil));
        hash_combine(seed, hash_value(v.blend));
        hash_combine(seed, std::hash<uint32_t>{}(static_cast<uint32_t>(v.topology)));
        hash_combine(seed, hash_bool(v.primitiveRestartEnable));
        hash_combine(seed, std::hash<uint32_t>{}(static_cast<uint32_t>(v.rasterizationSamples)));
        hash_combine(seed, hash_bool(v.sampleShadingEnable));
        hash_combine(seed, hash_span(v.dynamicStates));
        return seed;
    }
    
} // namespace AstralEngine

// ========== std::hash Specializations ==========

namespace std {
    template<> struct hash<AstralEngine::RasterizationConfig> {
        size_t operator()(const AstralEngine::RasterizationConfig& v) const noexcept { 
            return AstralEngine::hash_value(v); 
        }
    };
    
    template<> struct hash<AstralEngine::DepthStencilConfig> {
        size_t operator()(const AstralEngine::DepthStencilConfig& v) const noexcept { 
            return AstralEngine::hash_value(v); 
        }
    };
    
    template<> struct hash<AstralEngine::BlendConfig> {
        size_t operator()(const AstralEngine::BlendConfig& v) const noexcept { 
            return AstralEngine::hash_value(v); 
        }
    };
    
    template<> struct hash<AstralEngine::PipelineConfig> {
        size_t operator()(const AstralEngine::PipelineConfig& v) const noexcept { 
            return AstralEngine::hash_value(v); 
        }
    };
}
