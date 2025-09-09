#pragma once

#include "Platform/Window.h"
#include "Renderer/UnifiedMaterial.h"
#include "Renderer/PipelineConfig.h"
#include "ECS/ECS.h"
#include "ECS/Components.h"
#include <memory>
#include <vector>
#include <atomic>
#include <thread>
#include <chrono>
#include <unordered_map>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>

// Forward Declarations
namespace AstralEngine {
    namespace Vulkan {
        class VulkanDevice;
        class VulkanContext;
        class VulkanPipeline;
        class VulkanBuffer;
        class DescriptorSetManager;
    }
    class Model;
    class Shader;
    class UnifiedMaterialInstance;
    class MaterialShaderManager;
    class ShadowMapManager;
}

namespace AstralEngine {
    
    // Render queue entry for efficient sorting and batching
    struct RenderItem {
        ECS::EntityID entityId;
        std::shared_ptr<Model> model;
        std::shared_ptr<UnifiedMaterialInstance> material;
        glm::mat4 worldMatrix;
        float distanceToCamera;
        int renderQueue;
        uint64_t materialHash;
        
        // Constructor
        RenderItem(ECS::EntityID id, std::shared_ptr<Model> m, 
                  std::shared_ptr<UnifiedMaterialInstance> mat, 
                  const glm::mat4& worldMtx, float distance)
            : entityId(id), model(m), material(mat), worldMatrix(worldMtx), 
              distanceToCamera(distance) {
            if (material) {
                renderQueue = material->getRenderQueue();
                materialHash = material->getVariantHash();
            } else {
                renderQueue = 2000; // Default opaque queue
                materialHash = 0;
            }
        }
    };
    
    // Render batch for efficient rendering of similar materials
    struct RenderBatch {
        std::shared_ptr<UnifiedMaterialInstance> material;
        std::shared_ptr<Shader> shader;
        std::shared_ptr<Vulkan::VulkanPipeline> pipeline;
        std::vector<RenderItem*> items;
        uint64_t materialHash;
        int renderQueue;
        
        RenderBatch(std::shared_ptr<UnifiedMaterialInstance> mat)
            : material(mat) {
            if (material) {
                materialHash = material->getVariantHash();
                renderQueue = material->getRenderQueue();
            } else {
                materialHash = 0;
                renderQueue = 2000;
            }
        }
    };
    
    // ========== Pipeline Cache Key ==========
    
    /**
     * Composite key for pipeline caching that combines shader variant hash 
     * with pipeline configuration hash. This allows efficient pipeline reuse 
     * while properly handling different render states for the same shader.
     */
    struct PipelineCacheKey {
        uint64_t shaderVariantHash = 0;    // from UnifiedMaterialInstance::getVariantHash()
        uint64_t pipelineConfigHash = 0;   // from std::hash<PipelineConfig>()(config)
        
        bool operator==(const PipelineCacheKey& other) const noexcept {
            return shaderVariantHash == other.shaderVariantHash &&
                   pipelineConfigHash == other.pipelineConfigHash;
        }
    };
    
    // Hash functor for PipelineCacheKey
    struct PipelineCacheKeyHash {
        size_t operator()(const PipelineCacheKey& k) const noexcept {
            size_t seed = 0;
            hash_combine(seed, std::hash<uint64_t>{}(k.shaderVariantHash));
            hash_combine(seed, std::hash<uint64_t>{}(k.pipelineConfigHash));
            return seed;
        }
    };
    
    /**
     * Main renderer class responsible for Vulkan-based rendering.
     * Integrates with the ECS system to render entities with RenderComponent.
     */
    class Renderer {
    public:
        explicit Renderer(Window& window);
        ~Renderer();

        // Non-copyable and non-movable
        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;
        Renderer(Renderer&&) = delete;
        Renderer& operator=(Renderer&&) = delete;

        // Main rendering methods
        void drawFrame(const ECS::Scene& scene);
        void drawFrame(); // Backward compatibility with empty scene
        void onFramebufferResize();

        // Device access for external systems
        Vulkan::VulkanDevice& getDevice();
        const Vulkan::VulkanDevice& getDevice() const;

        // Renderer state queries
        bool isInitialized() const { return m_initialized; }
        bool isFramebufferResized() const { return m_framebufferResized; }
        bool canRender() const;
        
        // Statistics and debug info
        size_t getSwapChainImageCount() const;
        VkExtent2D getSwapChainExtent() const;
        void printDebugInfo() const;
        void logMemoryUsage() const;
        
        // Pipeline cache diagnostics
        void logPipelineCacheStats() const;
        void resetPipelineCacheStats();

    private:
        // Initialization and cleanup
        void initVulkan();
        void cleanup();
        void destroyDefaultWhiteTexture();
        void destroyDepthResources();
        void createInstance();
        void setupDebugMessenger();
        void createSurface();

        // Resource management
        void createDescriptorSetLayouts();
        void createUniformBuffers();
        void createDescriptorPool();
        void createDescriptorSets();
        void createMaterialDescriptorPool();
        void createMaterialDescriptorSets();
        void createDefaultWhiteTexture();
        void createDepthResources();
        void createCommandPool();
        void createCommandBuffers();
        
        // Rendering pipeline
        void recordCommandBuffer(uint32_t imageIndex, const ECS::Scene& scene);
        void recordCommandBuffer(uint32_t imageIndex); // Backward compatibility
        void updateUniformBuffer(uint32_t currentImage, const ECS::Scene& scene);
        void renderECSEntities(VkCommandBuffer commandBuffer, const ECS::Scene& scene, uint32_t imageIndex);
        
        // PBR Material rendering (optimized with frame allocator)
        void collectRenderItems(const ECS::Scene& scene, RenderItem* outItems, size_t& outCount, size_t maxItems);
        void createRenderBatches(const RenderItem* items, size_t itemCount, RenderBatch* outBatches, size_t& outBatchCount, size_t maxBatches);
        void sortRenderBatches(RenderBatch* batches, size_t batchCount);
        void renderBatches(VkCommandBuffer commandBuffer, const RenderBatch* batches, size_t batchCount, uint32_t imageIndex);
        
        // Shadow mapping
        void renderShadowMaps(VkCommandBuffer commandBuffer, const ECS::Scene& scene);
        
        // Swapchain management
        void recreateSwapChain();
        void cleanupSwapChain();
        
        // Utility functions
        VkFormat findSupportedDepthFormat() const;
        void transitionImageLayout(VkImage image, VkFormat format, 
                                 VkImageLayout oldLayout, VkImageLayout newLayout);
        bool waitForDeviceIdle(uint32_t timeoutMs = 5000);
        void recreateSwapChainBackground();

        // Core systems
        Window& m_window;
        std::unique_ptr<Vulkan::VulkanContext> m_context;
        std::unique_ptr<Vulkan::DescriptorSetManager> m_descriptorManager;
        std::unique_ptr<MaterialShaderManager> m_shaderManager;
        std::unique_ptr<ShadowMapManager> m_shadowManager;
        std::unique_ptr<Shader> m_shader;
        std::unique_ptr<Vulkan::VulkanPipeline> m_pipeline;
        std::unique_ptr<Model> m_defaultModel; // For testing/fallback
        // Camera is now managed via ECS - use scene.getMainCamera()
        
        // Pipeline cache for different material variants (shader + pipeline state combination)
        std::unordered_map<PipelineCacheKey, std::shared_ptr<Vulkan::VulkanPipeline>, PipelineCacheKeyHash> m_pipelineCache;

        // Vulkan resources
        std::vector<Vulkan::VulkanBuffer> m_uniformBuffers;
        std::vector<Vulkan::VulkanBuffer> m_defaultMaterialBuffers;
        
        // Descriptor sets
        VkDescriptorSetLayout m_sceneSetLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_materialSetLayout = VK_NULL_HANDLE;
        VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
        VkDescriptorPool m_materialDescriptorPool = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> m_descriptorSets; // Scene data (set 0)
        std::vector<VkDescriptorSet> m_materialDescriptorSets; // Material data (set 1)
        
        // Depth buffering
        VkFormat m_depthFormat = VK_FORMAT_D32_SFLOAT;
        std::vector<VkImage> m_depthImages;
        std::vector<VmaAllocation> m_depthImageAllocations;
        std::vector<VkImageView> m_depthImageViews;
        
        // Command recording
        VkCommandPool m_commandPool = VK_NULL_HANDLE;
        std::vector<VkCommandBuffer> m_commandBuffers;
        
        // Default resources
        VkImage m_defaultWhiteImage = VK_NULL_HANDLE;
        VmaAllocation m_defaultWhiteImageAllocation = VK_NULL_HANDLE;
        VkImageView m_defaultWhiteImageView = VK_NULL_HANDLE;
        VkSampler m_defaultWhiteTextureSampler = VK_NULL_HANDLE;

        // State flags
        bool m_initialized = false;
        bool m_framebufferResized = false;
        bool m_commandBuffersDirty = true;
        std::atomic<bool> m_recreatingSwapChain{false};
        std::atomic<bool> m_shouldExit{false};
        
        // Error tracking
        std::atomic<int> m_consecutiveErrors{0};
        static constexpr int MAX_CONSECUTIVE_ERRORS = 100;
        
        // Pipeline cache diagnostics (debug builds)
        mutable std::atomic<uint32_t> m_pipelineCacheHits{0};
        mutable std::atomic<uint32_t> m_pipelineCacheMisses{0};
        mutable std::atomic<uint32_t> m_pipelineCreationErrors{0};
    };

} // namespace AstralEngine

