#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <array>

// Forward declarations
namespace AstralEngine {
    namespace Vulkan {
        class VulkanDevice;
        class VulkanBuffer;
    }
    class Camera; // Legacy Camera forward declaration
    namespace ECS {
        struct Light;
        struct Camera;
        struct Transform;
    }
}

namespace AstralEngine {

    // Shadow mapping configuration
    struct ShadowConfig {
        // Cascade Shadow Maps (CSM) configuration
        uint32_t cascadeCount = 4;
        uint32_t shadowMapSize = 2048;
        float cascadeSplitLambda = 0.75f; // CSM split distribution
        
        // Shadow quality settings
        enum class FilterMode {
            Hard,           // Simple shadow test
            PCF,           // Percentage Closer Filtering
            PCSS,          // Percentage Closer Soft Shadows
            VSM            // Variance Shadow Maps
        };
        FilterMode filterMode = FilterMode::PCF;
        
        // PCF settings
        uint32_t pcfSampleCount = 9;
        float pcfRadius = 2.0f;
        
        // Bias settings to prevent shadow acne
        float depthBiasConstant = 1.25f;
        float depthBiasSlope = 1.75f;
        float normalOffsetScale = 0.01f;
        
        // Shadow distance and fade
        float maxShadowDistance = 100.0f;
        float fadeStartDistance = 80.0f;
        
        // Cascade debugging
        bool visualizeCascades = false;
    };

    // Shadow cascade information
    struct ShadowCascade {
        glm::mat4 viewMatrix;
        glm::mat4 projMatrix;
        glm::mat4 viewProjMatrix;
        float splitDistance;
        glm::vec4 boundingSphere; // xyz = center, w = radius
    };

    // Light shadow data
    struct DirectionalLightShadow {
        std::array<ShadowCascade, 4> cascades;
        uint32_t activeCascadeCount;
        glm::vec3 lightDirection;
        float shadowStrength = 1.0f;
    };

    // Shadow mapping GPU uniform buffer
    struct alignas(16) ShadowUBO {
        alignas(16) glm::mat4 lightSpaceMatrices[4]; // Up to 4 cascades
        alignas(16) glm::vec4 cascadeDistances;      // x,y,z,w = cascade split distances
        alignas(16) glm::vec4 shadowParams;          // x=bias, y=normal_offset, z=pcf_radius, w=strength
        alignas(16) glm::uvec4 shadowConfig;         // x=cascade_count, y=filter_mode, z=map_size, w=debug
        alignas(16) glm::vec3 lightDirection;       float _padding;
        
        ShadowUBO() {
            cascadeDistances = glm::vec4(0.0f);
            shadowParams = glm::vec4(0.0f);
            shadowConfig = glm::uvec4(0);
            lightDirection = glm::vec3(0.0f, -1.0f, 0.0f);
            _padding = 0.0f;
            
            for (int i = 0; i < 4; ++i) {
                lightSpaceMatrices[i] = glm::mat4(1.0f);
            }
        }
    };

    // Shadow map manager
    class ShadowMapManager {
    public:
        ShadowMapManager(Vulkan::VulkanDevice& device);
        ~ShadowMapManager();

        // Initialization
        bool initialize(const ShadowConfig& config);
        void shutdown();

        // Shadow map rendering
        void beginShadowPass(VkCommandBuffer commandBuffer, uint32_t cascadeIndex);
        void endShadowPass(VkCommandBuffer commandBuffer);
        
        // Shadow pipeline access
        void bindShadowPipeline(VkCommandBuffer commandBuffer);
        VkPipelineLayout getShadowPipelineLayout() const;
        
        // Cascade calculation
        void calculateDirectionalLightCascades(
            const ECS::Light& directionalLight,
            const ECS::Camera& camera,
            const ECS::Transform& cameraTransform,
            DirectionalLightShadow& shadowData
        );
        
        // Shadow data updates
        void updateShadowUBO(const DirectionalLightShadow& shadowData);
        
        // Resource access
        VkImage getShadowMapImage() const { return m_shadowMapImage; }
        VkImageView getShadowMapView() const { return m_shadowMapView; }
        VkSampler getShadowSampler() const { return m_shadowSampler; }
        const Vulkan::VulkanBuffer& getShadowUBO() const { return *m_shadowUBO; }
        
        // Configuration
        void setConfig(const ShadowConfig& config);
        const ShadowConfig& getConfig() const { return m_config; }
        
        // Debug utilities
        void setVisualizeCascades(bool enable) { m_config.visualizeCascades = enable; }
        void printDebugInfo() const;

    private:
        void createShadowMap();
        void createShadowSampler();
        void createShadowUBO();
        void createRenderPass();
        void createFramebuffers();
        void createShadowPipeline();
        
        // Cascade calculation helpers
        std::vector<float> calculateCascadeSplits(float nearPlane, float farPlane) const;
        glm::mat4 calculateLightViewMatrix(const glm::vec3& lightDirection, const glm::vec3& sceneCenter) const;
        glm::mat4 calculateLightProjectionMatrix(
            const glm::mat4& lightViewMatrix,
            const std::vector<glm::vec3>& frustumCorners
        ) const;
        
        // Frustum calculation (DEPRECATED: Use ShadowUtils::getFrustumCornersWorldSpace)
        std::vector<glm::vec3> getFrustumCornersWorldSpace(
            const glm::mat4& proj, 
            const glm::mat4& view,
            float nearPlane,
            float farPlane
        ) const;

        Vulkan::VulkanDevice& m_device;
        ShadowConfig m_config;
        
        // Shadow map resources
        VkImage m_shadowMapImage = VK_NULL_HANDLE;
        VmaAllocation m_shadowMapAllocation = VK_NULL_HANDLE;
        VkImageView m_shadowMapView = VK_NULL_HANDLE;
        VkSampler m_shadowSampler = VK_NULL_HANDLE;
        
        // Render pass and framebuffers
        VkRenderPass m_renderPass = VK_NULL_HANDLE;
        std::vector<VkFramebuffer> m_framebuffers;
        std::vector<VkImageView> m_cascadeImageViews; // Individual views for each cascade
        
        // Shadow pipeline
        VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
        VkPipeline m_pipeline = VK_NULL_HANDLE;
        
        // Shadow UBO
        std::unique_ptr<Vulkan::VulkanBuffer> m_shadowUBO;
        
        // Shadow map array (for multiple cascades)
        bool m_useArrayTexture = true;
        uint32_t m_currentCascade = 0;
    };

    // Configuration for tight-fitting cascade shadow mapping
    struct CascadeSettings {
        float splitLambda = 0.75f;          // Logarithmic/uniform split blend [0,1]
        float zPadding = 10.0f;             // Extra depth range for light frustum
        bool  tightZ = false;               // Use tight Z bounds (may shimmer more)
        bool  enableTexelSnapping = true;   // Snap light matrix to texel grid
        float aabbEpsilon = 0.5f;          // Expand light AABB by this amount
        glm::vec3 worldUp = {0.0f, 0.0f, 1.0f}; // World up vector (Z-up by default)
    };

    // Utility functions for shadow calculations
    namespace ShadowUtils {
        
        // Calculate optimal cascade split distances using practical split scheme
        std::vector<float> computeCascadeSplits(
            float nearPlane, 
            float farPlane, 
            uint32_t cascadeCount,
            float lambda = 0.75f
        );
        
        // Calculate frustum corners for a camera view slice
        std::array<glm::vec3, 8> getFrustumCornersWorldSpace(
            const Camera& camera,
            float splitNear,
            float splitFar
        );
        
        // Build light-space view matrix centered on frustum
        glm::mat4 buildDirectionalLightView(
            const glm::vec3& lightDirection,
            const std::array<glm::vec3, 8>& frustumCorners,
            const glm::vec3& worldUp = {0.0f, 0.0f, 1.0f}
        );
        
        // Compute tight AABB of frustum corners in light space
        void computeLightSpaceAABB(
            const std::array<glm::vec3, 8>& frustumCorners,
            const glm::mat4& lightView,
            glm::vec3& outMinLS,
            glm::vec3& outMaxLS
        );
        
        // Create tight orthographic projection from light-space AABB
        glm::mat4 buildTightOrthoFromAABB(
            const glm::vec3& minLS,
            const glm::vec3& maxLS,
            float zPadding,
            bool tightZ = false
        );
        
        // Apply texel snapping to reduce shadow shimmering
        void applyTexelSnapping(
            glm::mat4& lightView,
            glm::mat4& lightProj,
            uint32_t shadowMapResolution
        );
        
        // Calculate light space matrix for orthographic projection (UPGRADED)
        glm::mat4 calculateDirectionalLightMatrix(
            const Camera& camera,
            const glm::vec3& lightDirection,
            float splitNear,
            float splitFar,
            uint32_t shadowMapResolution,
            const CascadeSettings& settings = {}
        );
        
        // Legacy compatibility - deprecated, use upgraded version
        glm::mat4 calculateDirectionalLightMatrix(
            const glm::vec3& lightDirection,
            const std::vector<glm::vec3>& sceneCorners,
            float zMult = 10.0f
        );
        
        // Calculate bounding sphere for cascade optimization
        glm::vec4 calculateBoundingSphere(const std::vector<glm::vec3>& points);
        
        // Stabilize shadow cascades (reduce shimmer)
        glm::mat4 stabilizeProjectionMatrix(
            const glm::mat4& projMatrix,
            uint32_t shadowMapSize
        );
        
        // Debug visualization helpers
        std::vector<glm::vec3> getCascadeColors(uint32_t cascadeCount);
        void debugDrawCascadeFrustum(
            const ShadowCascade& cascade,
            const glm::vec3& color
        );
    }

    // Light shadow data container
    struct LightShadowData {
        enum class Type {
            None,
            Directional,
            Point,  // TODO: For future point light shadows
            Spot    // TODO: For future spot light shadows
        } type = Type::None;
        
        union {
            DirectionalLightShadow directional;
            // TODO: Add point and spot light shadow data
        };
        
        bool enabled = false;
        float intensity = 1.0f;
        
        LightShadowData() : type(Type::None), enabled(false) {}
        
        void setDirectional(const DirectionalLightShadow& data) {
            type = Type::Directional;
            directional = data;
            enabled = true;
        }
    };

    // Shadow rendering pipeline integration
    class ShadowRenderer {
    public:
        ShadowRenderer(Vulkan::VulkanDevice& device, ShadowMapManager& shadowManager);
        ~ShadowRenderer() = default;

        // Shadow pass rendering
        void renderShadowMaps(
            VkCommandBuffer commandBuffer,
            const std::vector<LightShadowData>& lightShadows,
            const std::vector<class RenderObject>& objects // TODO: Define RenderObject
        );
        
        // Shadow receiving
        void bindShadowResources(
            VkCommandBuffer commandBuffer,
            VkPipelineLayout pipelineLayout,
            uint32_t descriptorSetIndex
        );
        
    private:
        void renderDirectionalShadow(
            VkCommandBuffer commandBuffer,
            const DirectionalLightShadow& shadowData,
            const std::vector<class RenderObject>& objects
        );
        
        Vulkan::VulkanDevice& m_device;
        ShadowMapManager& m_shadowManager;
    };

} // namespace AstralEngine
