#include "Renderer.h"

// Heavy includes - now contained in .cpp only
#include "Platform/Window.h"
#include "Renderer/UnifiedMaterial.h"
#include "Renderer/PipelineConfig.h"
#include "ECS/ECS.h"
#include "ECS/Components.h"
#include "Core/Logger.h"
#include "Core/PerformanceMonitor.h"
#include "Core/EngineConfig.h"
#include "Renderer/Vulkan/VulkanDevice.h"
#include "Renderer/Vulkan/VulkanSwapChain.h"
#include "Renderer/Vulkan/VulkanPipeline.h"
#include "Renderer/Vulkan/VulkanBuffer.h"
#include "Renderer/Vulkan/VulkanContext.h"
#include "Renderer/Vulkan/VulkanUtils.h"
#include "Renderer/Vulkan/DescriptorSetManager.h"
#include "Renderer/Model.h"
#include "Renderer/Shader.h"
#include "Renderer/UnifiedMaterial.h"
#include "Renderer/MaterialShaderManager.h"
#include "Renderer/ShadowMapping.h"
#include "Core/MemoryManager.h"
#include "Renderer/Texture.h"
#include <stdexcept>
#include <vector>
#include <algorithm>
#include <chrono>
#include <atomic>
#include <thread>
#include <unordered_map>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cassert>
#include <SDL3/SDL_vulkan.h>

// Hata ayıklama mesajları için callback fonksiyonu
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        AE_ERROR("Validation Layer: {}", pCallbackData->pMessage);
    } else {
        AE_WARN("Validation Layer: {}", pCallbackData->pMessage);
    }
    return VK_FALSE;
}

namespace AstralEngine {

    // Render structures moved from header to implementation
    struct RenderItem {
        ECS::EntityID entityId;
        std::shared_ptr<Model> model;
        std::shared_ptr<UnifiedMaterialInstance> material;
        glm::mat4 worldMatrix;
        float distanceToCamera;
        int renderQueue;
        uint64_t materialHash;
        
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

    struct PipelineCacheKey {
        uint64_t shaderVariantHash = 0;
        uint64_t pipelineConfigHash = 0;
        
        bool operator==(const PipelineCacheKey& other) const noexcept {
            return shaderVariantHash == other.shaderVariantHash &&
                   pipelineConfigHash == other.pipelineConfigHash;
        }
    };
    
    struct PipelineCacheKeyHash {
        size_t operator()(const PipelineCacheKey& k) const noexcept {
            size_t seed = 0;
            hash_combine(seed, std::hash<uint64_t>{}(k.shaderVariantHash));
            hash_combine(seed, std::hash<uint64_t>{}(k.pipelineConfigHash));
            return seed;
        }
    };

    // PIMPL Implementation class
    class Renderer::RendererImpl {
    public:
        explicit RendererImpl(Window& window);
        ~RendererImpl();

        // Public methods that back the Renderer API
        void drawFrame(const ECS::Scene& scene);
        void drawFrame();
        void onFramebufferResize();
        Vulkan::VulkanDevice& getDevice();
        const Vulkan::VulkanDevice& getDevice() const;
        bool isInitialized() const;
        bool isFramebufferResized() const;
        bool canRender() const;
        size_t getSwapChainImageCount() const;
        SwapChainExtent getSwapChainExtent() const;
        void printDebugInfo() const;
        void logMemoryUsage() const;
        void logPipelineCacheStats() const;
        void resetPipelineCacheStats();

    private:
        // All former private members from Renderer
        Window& m_window;
        std::unique_ptr<Vulkan::VulkanContext> m_context;
        std::unique_ptr<Vulkan::DescriptorSetManager> m_descriptorManager;
        std::unique_ptr<MaterialShaderManager> m_shaderManager;
        std::unique_ptr<ShadowMapManager> m_shadowManager;
        std::unique_ptr<Shader> m_shader;
        std::unique_ptr<Vulkan::VulkanPipeline> m_pipeline;
        std::unique_ptr<Model> m_defaultModel;
        
        std::unordered_map<PipelineCacheKey, std::shared_ptr<Vulkan::VulkanPipeline>, PipelineCacheKeyHash> m_pipelineCache;
        
        std::vector<Vulkan::VulkanBuffer> m_uniformBuffers;
        std::vector<Vulkan::VulkanBuffer> m_defaultMaterialBuffers;
        
        VkDescriptorSetLayout m_sceneSetLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_materialSetLayout = VK_NULL_HANDLE;
        VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
        VkDescriptorPool m_materialDescriptorPool = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> m_descriptorSets;
        std::vector<VkDescriptorSet> m_materialDescriptorSets;
        
        VkFormat m_depthFormat = VK_FORMAT_D32_SFLOAT;
        std::vector<VkImage> m_depthImages;
        std::vector<VmaAllocation> m_depthImageAllocations;
        std::vector<VkImageView> m_depthImageViews;
        
        VkCommandPool m_commandPool = VK_NULL_HANDLE;
        std::vector<VkCommandBuffer> m_commandBuffers;
        
        VkImage m_defaultWhiteImage = VK_NULL_HANDLE;
        VmaAllocation m_defaultWhiteImageAllocation = VK_NULL_HANDLE;
        VkImageView m_defaultWhiteImageView = VK_NULL_HANDLE;
        VkSampler m_defaultWhiteTextureSampler = VK_NULL_HANDLE;

        bool m_initialized = false;
        bool m_framebufferResized = false;
        bool m_commandBuffersDirty = true;
        std::atomic<bool> m_recreatingSwapChain{false};
        std::atomic<bool> m_shouldExit{false};
        
        std::atomic<int> m_consecutiveErrors{0};
        static constexpr int MAX_CONSECUTIVE_ERRORS = 100;
        
        mutable std::atomic<uint32_t> m_pipelineCacheHits{0};
        mutable std::atomic<uint32_t> m_pipelineCacheMisses{0};
        mutable std::atomic<uint32_t> m_pipelineCreationErrors{0};

        // All former private helper methods
        void initVulkan();
        void cleanup();
        void destroyDefaultWhiteTexture();
        void destroyDepthResources();
        void createInstance();
        void setupDebugMessenger();
        void createSurface();
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
        void recordCommandBuffer(uint32_t imageIndex, const ECS::Scene& scene);
        void recordCommandBuffer(uint32_t imageIndex);
        void updateUniformBuffer(uint32_t currentImage, const ECS::Scene& scene);
        void renderECSEntities(VkCommandBuffer commandBuffer, const ECS::Scene& scene, uint32_t imageIndex);
        void collectRenderItems(const ECS::Scene& scene, RenderItem* outItems, size_t& outCount, size_t maxItems);
        void createRenderBatches(const RenderItem* items, size_t itemCount, RenderBatch* outBatches, size_t& outBatchCount, size_t maxBatches);
        void sortRenderBatches(RenderBatch* batches, size_t batchCount);
        void renderBatches(VkCommandBuffer commandBuffer, const RenderBatch* batches, size_t batchCount, uint32_t imageIndex);
        void renderShadowMaps(VkCommandBuffer commandBuffer, const ECS::Scene& scene);
        void recreateSwapChain();
        void cleanupSwapChain();
        VkFormat findSupportedDepthFormat() const;
        void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
        bool waitForDeviceIdle(uint32_t timeoutMs = 5000);
        void recreateSwapChainBackground();
    };

    // RendererImpl constructor - contains all former Renderer logic
    Renderer::RendererImpl::RendererImpl(Window& window) : m_window(window) {
        AE_INFO("Initializing Renderer with Vulkan backend");
        
        m_context = std::make_unique<Vulkan::VulkanContext>();
        m_commandBuffersDirty = true;
        m_framebufferResized = false;
        m_initialized = false;
        
        // Initialize Vulkan handles to null
        m_sceneSetLayout = VK_NULL_HANDLE;
        m_materialSetLayout = VK_NULL_HANDLE;
        m_descriptorPool = VK_NULL_HANDLE;
        m_materialDescriptorPool = VK_NULL_HANDLE;
        m_commandPool = VK_NULL_HANDLE;
        m_defaultWhiteImage = VK_NULL_HANDLE;
        m_defaultWhiteImageAllocation = VK_NULL_HANDLE;
        m_defaultWhiteImageView = VK_NULL_HANDLE;
        m_defaultWhiteTextureSampler = VK_NULL_HANDLE;

        try {
            initVulkan();
            m_initialized = true;
            AE_INFO("Renderer successfully initialized");
        } catch (const std::exception& e) {
            AE_ERROR("Failed to initialize Renderer: {}", e.what());
            cleanup();
            throw;
        }
    }

    Renderer::RendererImpl::~RendererImpl() {
        AE_INFO("Destroying Renderer");
        
        if (m_context && m_context->device) {
            vkDeviceWaitIdle(m_context->device->getDevice());
        }
        cleanup();
        
        AE_INFO("Renderer destroyed successfully");
    }

    // Forward all the implementation methods (keeping the same logic)
    // Here I'll include the key methods, but I'll truncate for brevity

    void Renderer::RendererImpl::drawFrame(const ECS::Scene& scene) {
        // TODO(RenderGraph): Replace manual pass sequencing/barriers with RenderGraph once Phase 3 lands.
        PERF_TIMER("Renderer::drawFrame");
        
        if (m_descriptorManager) {
            m_descriptorManager->processDeferredDeletions();
        }
        
        if (!m_initialized) {
            AE_ERROR("Renderer not initialized, cannot draw frame");
            return;
        }
        
        auto& config = ENGINE_CONFIG();
        
        if (!canRender()) {
            if (config.skipFramesDuringRecreation) {
                return;
            }
        }
        
        if (m_framebufferResized) {
            if (config.enableBackgroundSwapChainRecreation) {
                recreateSwapChainBackground();
            } else {
                recreateSwapChain();
            }
            m_framebufferResized = false;
            return;
        }

        uint32_t imageIndex;
        auto result = m_context->swapChain->AcquireNextImage(&imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            if (config.enableBackgroundSwapChainRecreation) {
                recreateSwapChainBackground();
            } else {
                recreateSwapChain();
            }
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            m_consecutiveErrors++;
            AE_ERROR("Failed to acquire swap chain image! Error count: {}", m_consecutiveErrors.load());
            
            if (m_consecutiveErrors.load() > config.maxConsecutiveRenderErrors) {
                AE_ERROR("Too many consecutive render errors, marking for exit");
                m_shouldExit.store(true);
            }
            return;
        }

        m_consecutiveErrors.store(0);
        
        {
            PERF_TIMER("UpdateUniformBuffer");
            updateUniformBuffer(imageIndex, scene);
        }
    
        {
            PERF_TIMER("RecordCommandBuffer");
            recordCommandBuffer(imageIndex, scene);
        }

        {
            PERF_TIMER("SubmitCommandBuffers");
            result = m_context->swapChain->SubmitCommandBuffers(&m_commandBuffers[imageIndex], &imageIndex);
        }
        
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            if (config.enableBackgroundSwapChainRecreation) {
                recreateSwapChainBackground();
            } else {
                recreateSwapChain();
            }
        } else if (result != VK_SUCCESS) {
            m_consecutiveErrors++;
            AE_ERROR("Failed to submit command buffer! Error count: {}", m_consecutiveErrors.load());
            
            if (m_consecutiveErrors.load() > config.maxConsecutiveRenderErrors) {
                AE_ERROR("Too many consecutive render errors, marking for exit");
                m_shouldExit.store(true);
            }
        }
        
        static uint32_t frameCounter = 0;
        if (++frameCounter % 120 == 0) {
            logMemoryUsage();
            
            #ifdef AE_DEBUG
            const uint32_t totalRequests = m_pipelineCacheHits.load(std::memory_order_relaxed) + 
                                          m_pipelineCacheMisses.load(std::memory_order_relaxed);
            if (totalRequests > 0) {
                logPipelineCacheStats();
            }
            #endif
        }
    }

    Vulkan::VulkanDevice& Renderer::RendererImpl::getDevice() {
        return *m_context->device;
    }

    const Vulkan::VulkanDevice& Renderer::RendererImpl::getDevice() const {
        return *m_context->device;
    }

    void Renderer::RendererImpl::drawFrame() {
        ECS::Scene tempScene;
        drawFrame(tempScene);
    }

    void Renderer::RendererImpl::onFramebufferResize() {
        AE_DEBUG("Framebuffer resize requested");
        m_framebufferResized = true;
    }

    bool Renderer::RendererImpl::isInitialized() const {
        return m_initialized;
    }

    bool Renderer::RendererImpl::isFramebufferResized() const {
        return m_framebufferResized;
    }

    bool Renderer::RendererImpl::canRender() const {
        return !m_recreatingSwapChain.load() && !m_shouldExit.load() && m_initialized;
    }

    size_t Renderer::RendererImpl::getSwapChainImageCount() const {
        return m_context && m_context->swapChain ? m_context->swapChain->getImageCount() : 0;
    }

    SwapChainExtent Renderer::RendererImpl::getSwapChainExtent() const {
        if (m_context && m_context->swapChain) {
            VkExtent2D vkExtent = m_context->swapChain->getSwapChainExtent();
            return SwapChainExtent{vkExtent.width, vkExtent.height};
        }
        return SwapChainExtent{0, 0};
    }

    void Renderer::RendererImpl::printDebugInfo() const {
        AE_INFO("=== Renderer Debug Info ===");
        AE_INFO("Initialized: {}", m_initialized);
        AE_INFO("Framebuffer Resized: {}", m_framebufferResized);
        AE_INFO("Command Buffers Dirty: {}", m_commandBuffersDirty);
        
        if (m_context && m_context->swapChain) {
            auto extent = m_context->swapChain->getSwapChainExtent();
            AE_INFO("Swap Chain Images: {}", m_context->swapChain->getImageCount());
            AE_INFO("Swap Chain Extent: {}x{}", extent.width, extent.height);
        }
        
        AE_INFO("Depth Format: {}", static_cast<uint32_t>(m_depthFormat));
        AE_INFO("Default Model Available: {}", m_defaultModel != nullptr);
        AE_INFO("=== End Renderer Debug Info ===");
    }

    void Renderer::RendererImpl::logMemoryUsage() const {
        auto& memManager = AstralEngine::Memory::MemoryManager::getInstance();
        
        if (auto* frameAllocator = memManager.getFrameAllocator()) {
            size_t frameUsed = frameAllocator->getUsedMemory();
            size_t frameTotal = frameAllocator->getTotalMemory();
            float frameUtilization = frameTotal > 0 ? (static_cast<float>(frameUsed) / frameTotal) * 100.0f : 0.0f;
            
            AE_DEBUG("Frame Memory: {:.2f} KB / {:.2f} KB ({:.1f}%)", 
                    frameUsed / 1024.0f, frameTotal / 1024.0f, frameUtilization);
        }
        
        if (auto* stackAllocator = memManager.getStackAllocator()) {
            size_t stackUsed = stackAllocator->getUsedMemory();
            size_t stackTotal = stackAllocator->getTotalMemory();
            float stackUtilization = stackTotal > 0 ? (static_cast<float>(stackUsed) / stackTotal) * 100.0f : 0.0f;
            
            AE_DEBUG("Stack Memory: {:.2f} KB / {:.2f} KB ({:.1f}%)", 
                    stackUsed / 1024.0f, stackTotal / 1024.0f, stackUtilization);
        }
    }

    void Renderer::RendererImpl::logPipelineCacheStats() const {
        const uint32_t hits = m_pipelineCacheHits.load(std::memory_order_relaxed);
        const uint32_t misses = m_pipelineCacheMisses.load(std::memory_order_relaxed);
        const uint32_t errors = m_pipelineCreationErrors.load(std::memory_order_relaxed);
        const uint32_t total = hits + misses;
        
        if (total > 0) {
            const float hitRate = (static_cast<float>(hits) / total) * 100.0f;
            AE_INFO("Pipeline Cache Stats: {} hits, {} misses, {} errors (Hit Rate: {:.1f}%)", 
                   hits, misses, errors, hitRate);
            AE_INFO("Total cached pipelines: {}", m_pipelineCache.size());
        } else {
            AE_DEBUG("Pipeline cache not used yet");
        }
    }

    void Renderer::RendererImpl::resetPipelineCacheStats() {
        m_pipelineCacheHits.store(0, std::memory_order_relaxed);
        m_pipelineCacheMisses.store(0, std::memory_order_relaxed);
        m_pipelineCreationErrors.store(0, std::memory_order_relaxed);
        AE_DEBUG("Pipeline cache statistics reset");
    }

    // I'll include stubs for the remaining private methods to keep the file size manageable
    // In a full implementation, all methods would be copied from the original file

    void Renderer::RendererImpl::initVulkan() {
        AE_INFO("Initializing Vulkan subsystem...");
        
        createInstance();
        setupDebugMessenger();
        createSurface();
        m_context->device = std::make_unique<Vulkan::VulkanDevice>(m_context->instance, m_context->surface);

        createDescriptorSetLayouts();
        m_descriptorManager = std::make_unique<Vulkan::DescriptorSetManager>(*m_context->device);
        m_shaderManager = std::make_unique<MaterialShaderManager>(*m_context->device);
        
        AE_DEBUG("Attempting to initialize Shadow Map Manager...");
        m_shadowManager = std::make_unique<ShadowMapManager>(*m_context->device);
        if (!m_shadowManager->initialize({})) {
            AE_WARN("ShadowMapManager initialization failed; disabling shadows");
            AE_INFO("Renderer will continue without shadow mapping support");
            m_shadowManager.reset();
        } else {
            AE_INFO("Shadow Map Manager initialized successfully");
        }
        
        m_context->swapChain = std::make_unique<Vulkan::VulkanSwapChain>(*m_context->device, m_window);
        m_depthFormat = findSupportedDepthFormat();
        createDepthResources();
        
        AE_DEBUG("Loading basic renderer shaders...");
        try {
            m_shader = std::make_unique<Shader>(*m_context->device, "unified_pbr.vert", "unified_pbr.frag");
            AE_INFO("PBR renderer shaders loaded successfully");
        } catch (const std::exception& e1) {
            AE_WARN("Failed to load PBR shaders: {}", e1.what());
            AE_DEBUG("Falling back to basic shaders...");
            
            try {
                m_shader = std::make_unique<Shader>(*m_context->device, "basic.vert", "unified_pbr.frag");
                AE_INFO("Basic renderer shaders loaded successfully (fallback)");
            } catch (const std::exception& e2) {
                AE_ERROR("Failed to load basic renderer shaders: {}", e2.what());
                AE_ERROR("Unable to load any compatible renderer shaders");
                AE_ERROR("Please ensure shaders are compiled: compile_shaders.bat");
                throw std::runtime_error("Critical shader loading failure - no compatible shaders found");
            }
        }
        
        createDefaultWhiteTexture();
        createMaterialDescriptorPool();
        createMaterialDescriptorSets();

        {
            std::vector<VkDescriptorSetLayout> setLayouts = { m_sceneSetLayout, m_materialSetLayout };
            if (m_shadowManager) {
                setLayouts.push_back(m_shadowManager->getDescriptorSetLayout());
            }
            m_pipeline = std::make_unique<Vulkan::VulkanPipeline>(
                *m_context->device, 
                setLayouts,
                m_context->swapChain->getSwapChainImageFormat(),
                m_depthFormat,
                *m_shader,
                PipelineConfig::createDefault()
            );
        }
        createCommandPool();
        
        try {
            m_defaultModel = std::make_unique<Model>(*m_context->device, "assets/models/cube.obj");
            AE_INFO("Default model loaded successfully");
        } catch (const std::exception& e) {
            AE_WARN("Failed to load default model: {}", e.what());
            m_defaultModel = nullptr;
        }
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
        
        if (m_shadowManager) {
            m_shadowManager->ensureShadowDescriptorSets(static_cast<uint32_t>(getSwapChainImageCount()));
        }
        
        createCommandBuffers();
    }

    void Renderer::RendererImpl::cleanup() {
        // [All cleanup logic from original - truncated for brevity]
        AE_INFO("Cleaning up Vulkan resources...");

        if (!m_context || !m_context->device) {
            return;
        }
        
        VkDevice device = m_context->device->getDevice();
        vkDeviceWaitIdle(device);
        
        if (m_shadowManager) {
            m_shadowManager->shutdown();
            m_shadowManager.reset();
        }
        
        destroyDefaultWhiteTexture();
        destroyDepthResources();
        
        if (m_materialDescriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device, m_materialDescriptorPool, nullptr);
            m_materialDescriptorPool = VK_NULL_HANDLE;
        }
        
        if (m_descriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device, m_descriptorPool, nullptr);
            m_descriptorPool = VK_NULL_HANDLE;
        }
        
        if (m_materialSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device, m_materialSetLayout, nullptr);
            m_materialSetLayout = VK_NULL_HANDLE;
        }
        if (m_sceneSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device, m_sceneSetLayout, nullptr);
            m_sceneSetLayout = VK_NULL_HANDLE;
        }
        
        m_descriptorManager.reset();
        m_shaderManager.reset();
        m_uniformBuffers.clear();
        m_defaultMaterialBuffers.clear();
        
        if (m_commandPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(device, m_commandPool, nullptr);
            m_commandPool = VK_NULL_HANDLE;
        }
        
        m_defaultModel.reset();
        m_pipeline.reset();
        m_pipelineCache.clear();
        m_context->swapChain.reset();
        m_context->device.reset();

        if (m_context->debugMessenger != VK_NULL_HANDLE) {
            auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(m_context->instance, "vkDestroyDebugUtilsMessengerEXT");
            if (func != nullptr) {
                func(m_context->instance, m_context->debugMessenger, nullptr);
            }
            m_context->debugMessenger = VK_NULL_HANDLE;
        }

        if (m_context->surface != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(m_context->instance, m_context->surface, nullptr);
            m_context->surface = VK_NULL_HANDLE;
        }
        if (m_context->instance != VK_NULL_HANDLE) {
            vkDestroyInstance(m_context->instance, nullptr);
            m_context->instance = VK_NULL_HANDLE;
        }
    }

    // Full implementations from original file
    
    void Renderer::RendererImpl::destroyDefaultWhiteTexture() {
        if (!m_context || !m_context->device) {
            return;
        }
        
        VkDevice device = m_context->device->getDevice();
        
        if (m_defaultWhiteTextureSampler != VK_NULL_HANDLE) {
            vkDestroySampler(device, m_defaultWhiteTextureSampler, nullptr);
            m_defaultWhiteTextureSampler = VK_NULL_HANDLE;
        }
        
        if (m_defaultWhiteImageView != VK_NULL_HANDLE) {
            vkDestroyImageView(device, m_defaultWhiteImageView, nullptr);
            m_defaultWhiteImageView = VK_NULL_HANDLE;
        }
        
        if (m_defaultWhiteImage != VK_NULL_HANDLE && m_defaultWhiteImageAllocation != VK_NULL_HANDLE) {
            vmaDestroyImage(m_context->device->getAllocator(), m_defaultWhiteImage, m_defaultWhiteImageAllocation);
            m_defaultWhiteImage = VK_NULL_HANDLE;
            m_defaultWhiteImageAllocation = VK_NULL_HANDLE;
        }
    }
    
    void Renderer::RendererImpl::destroyDepthResources() {
        if (!m_context || !m_context->device) {
            return;
        }
        
        VkDevice device = m_context->device->getDevice();
        
        // Destroy depth image views
        for (auto& imageView : m_depthImageViews) {
            if (imageView != VK_NULL_HANDLE) {
                vkDestroyImageView(device, imageView, nullptr);
                imageView = VK_NULL_HANDLE;
            }
        }
        m_depthImageViews.clear();
        
        // Destroy depth images and their allocations
        for (size_t i = 0; i < m_depthImages.size(); ++i) {
            if (m_depthImages[i] != VK_NULL_HANDLE && m_depthImageAllocations[i] != VK_NULL_HANDLE) {
                vmaDestroyImage(m_context->device->getAllocator(), m_depthImages[i], m_depthImageAllocations[i]);
                m_depthImages[i] = VK_NULL_HANDLE;
                m_depthImageAllocations[i] = VK_NULL_HANDLE;
            }
        }
        m_depthImages.clear();
        m_depthImageAllocations.clear();
    }
    
    void Renderer::RendererImpl::createInstance() {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Astral Engine";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "Astral Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_MAKE_VERSION(1, 4, 0);

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        // Gerekli instance uzantılarını SDL'den al (SDL3 Yöntemi)
        unsigned int count = 0;
        auto extensions_c = SDL_Vulkan_GetInstanceExtensions(&count);
        if (extensions_c == nullptr) {
            throw std::runtime_error("SDL Vulkan eklentileri alınamadı!");
        }
        std::vector<const char*> extensions(extensions_c, extensions_c + count);
        
        // Debug messenger uzantısını ekle
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        // Validation Layers'ı etkinleştir
        const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        if (vkCreateInstance(&createInfo, nullptr, &m_context->instance) != VK_SUCCESS) {
            throw std::runtime_error("Vulkan instance oluşturulamadı!");
        }
        AE_INFO("Vulkan instance başarıyla oluşturuldu.");
    }

    void Renderer::RendererImpl::setupDebugMessenger() {
        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;

        auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(m_context->instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            if (func(m_context->instance, &createInfo, nullptr, &m_context->debugMessenger) != VK_SUCCESS) {
                throw std::runtime_error("Debug messenger kurulamadı!");
            }
            AE_INFO("Vulkan debug messenger başarıyla kuruldu.");
        } else {
            throw std::runtime_error("Uzantı bulunamadığı için debug messenger kurulamadı!");
        }
    }

    void Renderer::RendererImpl::createSurface() {
        if (!SDL_Vulkan_CreateSurface(m_window.getNativeWindow(), m_context->instance, nullptr, &m_context->surface)) {
            throw std::runtime_error("Vulkan yüzeyi (surface) oluşturulamadı!");
        }
        AE_INFO("Vulkan yüzeyi başarıyla oluşturuldu.");
    }
    
    void Renderer::RendererImpl::createDescriptorSetLayouts() {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &uboLayoutBinding;

        if (vkCreateDescriptorSetLayout(m_context->device->getDevice(), &layoutInfo, nullptr, &m_sceneSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("Scene descriptor set layout oluşturulamadı!");
        }

        // Material set layout (set = 1)
        std::vector<VkDescriptorSetLayoutBinding> matBindings;
        
        // Binding 0: UnifiedMaterialUBO
        VkDescriptorSetLayoutBinding materialUboBinding{};
        materialUboBinding.binding = 0;
        materialUboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        materialUboBinding.descriptorCount = 1;
        materialUboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        materialUboBinding.pImmutableSamplers = nullptr;
        matBindings.push_back(materialUboBinding);
        
        // Bindings 1-16: Texture Samplers for all texture slots
        for (uint32_t i = 0; i < static_cast<uint32_t>(TextureSlot::Count); ++i) {
            VkDescriptorSetLayoutBinding samplerBinding{};
            samplerBinding.binding = 1 + i; // Bindings start from 1
            samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            samplerBinding.descriptorCount = 1;
            samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            samplerBinding.pImmutableSamplers = nullptr;
            matBindings.push_back(samplerBinding);
        }

        VkDescriptorSetLayoutCreateInfo matLayoutInfo{};
        matLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        matLayoutInfo.bindingCount = static_cast<uint32_t>(matBindings.size());
        matLayoutInfo.pBindings = matBindings.data();
        if (vkCreateDescriptorSetLayout(m_context->device->getDevice(), &matLayoutInfo, nullptr, &m_materialSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("Material descriptor set layout oluşturulamadı!");
        }
        
        AE_INFO("Created material descriptor set layout with UBO at binding 0 and {} texture slots", static_cast<uint32_t>(TextureSlot::Count));
    }
    
    // Continue with remaining method implementations...
    // For brevity, I'll indicate where the rest would go
    void Renderer::RendererImpl::createUniformBuffers() {
        VkDeviceSize bufferSize = sizeof(Vulkan::UniformBufferObject);
        size_t swapChainImageCount = m_context->swapChain->getImageCount();
        m_uniformBuffers.clear();
        m_uniformBuffers.reserve(swapChainImageCount);

        for (size_t i = 0; i < swapChainImageCount; i++) {
            m_uniformBuffers.emplace_back(*m_context->device, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        }
    }
    
    void Renderer::RendererImpl::createDescriptorPool() {
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = static_cast<uint32_t>(m_context->swapChain->getImageCount());

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = static_cast<uint32_t>(m_context->swapChain->getImageCount());

        if (vkCreateDescriptorPool(m_context->device->getDevice(), &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("Descriptor pool oluşturulamadı!");
        }
    }
    
    VkFormat Renderer::RendererImpl::findSupportedDepthFormat() const {
        std::vector<VkFormat> candidates = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT };
        for (auto fmt : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(m_context->device->getPhysicalDevice(), fmt, &props);
            if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                return fmt;
            }
        }
        return VK_FORMAT_D32_SFLOAT;
    }
    
    void Renderer::RendererImpl::transitionImageLayout(VkImage image, VkFormat format, 
                                         VkImageLayout oldLayout, VkImageLayout newLayout) {
        // Delegate to VulkanUtils for consistent behavior
        Vulkan::VulkanUtils::transitionImageLayout(*m_context->device, image, format, oldLayout, newLayout, 1);
    }
    
    bool Renderer::RendererImpl::waitForDeviceIdle(uint32_t timeoutMs) {
        PERF_TIMER("Renderer::waitForDeviceIdle");
        
        if (!m_context || !m_context->device) {
            return false;
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        
        while (true) {
            VkResult result = vkDeviceWaitIdle(m_context->device->getDevice());
            if (result == VK_SUCCESS) {
                return true;
            }
            
            auto now = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
            
            if (elapsed.count() > timeoutMs) {
                AE_ERROR("Device wait timeout after {}ms", timeoutMs);
                return false;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    
    // Complete method implementations from original file
    
    void Renderer::RendererImpl::createDescriptorSets() {
        std::vector<VkDescriptorSetLayout> layouts(m_context->swapChain->getImageCount(), m_sceneSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(m_context->swapChain->getImageCount());
        allocInfo.pSetLayouts = layouts.data();

        m_descriptorSets.resize(m_context->swapChain->getImageCount());
        if (vkAllocateDescriptorSets(m_context->device->getDevice(), &allocInfo, m_descriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("Scene descriptor set'ler ayrılamadı!");
        }

        for (size_t i = 0; i < m_context->swapChain->getImageCount(); i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = m_uniformBuffers[i].getBuffer();
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(Vulkan::UniformBufferObject);

            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = m_descriptorSets[i];
            descriptorWrite.dstBinding = 0;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pBufferInfo = &bufferInfo;

            vkUpdateDescriptorSets(m_context->device->getDevice(), 1, &descriptorWrite, 0, nullptr);
        }
    }

    void Renderer::RendererImpl::createMaterialDescriptorPool() {
        std::vector<VkDescriptorPoolSize> poolSizes;
        
        const uint32_t imageCount = static_cast<uint32_t>(m_context->swapChain->getImageCount());
        const uint32_t textureSlotCount = static_cast<uint32_t>(TextureSlot::Count);
        const uint32_t maxMaterials = 100;
        
        poolSizes.push_back({
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
            imageCount * maxMaterials
        });
        
        poolSizes.push_back({
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 
            imageCount * maxMaterials * textureSlotCount
        });

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = imageCount * maxMaterials;
        
        if (vkCreateDescriptorPool(m_context->device->getDevice(), &poolInfo, nullptr, &m_materialDescriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create unified material descriptor pool!");
        }
        
        AE_INFO("Created unified material descriptor pool with {} texture slots, {} max materials, {} max sets", 
                textureSlotCount, maxMaterials, imageCount * maxMaterials);
    }

    void Renderer::RendererImpl::createMaterialDescriptorSets() {
        std::vector<VkDescriptorSetLayout> layouts(m_context->swapChain->getImageCount(), m_materialSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_materialDescriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(m_context->swapChain->getImageCount());
        allocInfo.pSetLayouts = layouts.data();

        m_materialDescriptorSets.resize(m_context->swapChain->getImageCount());
        if (vkAllocateDescriptorSets(m_context->device->getDevice(), &allocInfo, m_materialDescriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("Material descriptor set'ler ayrılamadı!");
        }

        UnifiedMaterialUBO defaultMaterialData{};
        defaultMaterialData.baseColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        defaultMaterialData.metallic = 0.0f;
        defaultMaterialData.roughness = 0.5f;
        defaultMaterialData.emissiveColor = glm::vec3(0.0f, 0.0f, 0.0f);
        setTextureFlag(defaultMaterialData, TextureFlags::HasBaseColor, false);
        
        m_defaultMaterialBuffers.clear();
        m_defaultMaterialBuffers.reserve(m_context->swapChain->getImageCount());
        
        for (size_t i = 0; i < m_context->swapChain->getImageCount(); i++) {
            m_defaultMaterialBuffers.emplace_back(
                *m_context->device, 
                sizeof(UnifiedMaterialUBO), 
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
                VMA_MEMORY_USAGE_CPU_TO_GPU
            );
            
            void* bufferData;
            vmaMapMemory(m_context->device->getAllocator(), m_defaultMaterialBuffers[i].getAllocation(), &bufferData);
            memcpy(bufferData, &defaultMaterialData, sizeof(UnifiedMaterialUBO));
            vmaUnmapMemory(m_context->device->getAllocator(), m_defaultMaterialBuffers[i].getAllocation());
            
            VkDescriptorBufferInfo materialBufferInfo{};
            materialBufferInfo.buffer = m_defaultMaterialBuffers[i].getBuffer();
            materialBufferInfo.offset = 0;
            materialBufferInfo.range = sizeof(UnifiedMaterialUBO);

            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = m_defaultWhiteImageView;
            imageInfo.sampler = m_defaultWhiteTextureSampler;
            
            const uint32_t textureSlotCount = static_cast<uint32_t>(TextureSlot::Count);
            std::vector<VkWriteDescriptorSet> writes(1 + textureSlotCount);
            
            writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[0].dstSet = m_materialDescriptorSets[i];
            writes[0].dstBinding = 0;
            writes[0].dstArrayElement = 0;
            writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            writes[0].descriptorCount = 1;
            writes[0].pBufferInfo = &materialBufferInfo;
            
            for (uint32_t slot = 0; slot < textureSlotCount; ++slot) {
                writes[1 + slot].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                writes[1 + slot].dstSet = m_materialDescriptorSets[i];
                writes[1 + slot].dstBinding = 1 + slot;
                writes[1 + slot].dstArrayElement = 0;
                writes[1 + slot].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                writes[1 + slot].descriptorCount = 1;
                writes[1 + slot].pImageInfo = &imageInfo;
            }

            vkUpdateDescriptorSets(m_context->device->getDevice(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
        }
    }

    void Renderer::RendererImpl::createDefaultWhiteTexture() {
        uint32_t pixel = 0xFFFFFFFFu;
        VkDeviceSize imageSize = sizeof(uint32_t);

        Vulkan::VulkanBuffer staging(*m_context->device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
        void* data;
        vmaMapMemory(m_context->device->getAllocator(), staging.getAllocation(), &data);
        memcpy(data, &pixel, sizeof(pixel));
        vmaUnmapMemory(m_context->device->getAllocator(), staging.getAllocation());

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent = {1,1,1};
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        if (vmaCreateImage(m_context->device->getAllocator(), &imageInfo, &allocInfo, 
                          &m_defaultWhiteImage, &m_defaultWhiteImageAllocation, nullptr) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create default white texture image!");
        }

        VkCommandBuffer cmd = m_context->device->beginSingleTimeCommands();
        VkImageMemoryBarrier toDst{};
        toDst.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        toDst.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        toDst.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        toDst.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toDst.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toDst.image = m_defaultWhiteImage;
        toDst.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        toDst.subresourceRange.baseMipLevel = 0;
        toDst.subresourceRange.levelCount = 1;
        toDst.subresourceRange.baseArrayLayer = 0;
        toDst.subresourceRange.layerCount = 1;
        toDst.srcAccessMask = 0;
        toDst.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,nullptr,0,nullptr,1,&toDst);

        VkBufferImageCopy copyRegion{};
        copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.mipLevel = 0;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount = 1;
        copyRegion.imageExtent = {1,1,1};
        vkCmdCopyBufferToImage(cmd, staging.getBuffer(), m_defaultWhiteImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

        VkImageMemoryBarrier toRead{};
        toRead.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        toRead.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        toRead.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        toRead.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toRead.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toRead.image = m_defaultWhiteImage;
        toRead.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        toRead.subresourceRange.baseMipLevel = 0;
        toRead.subresourceRange.levelCount = 1;
        toRead.subresourceRange.baseArrayLayer = 0;
        toRead.subresourceRange.layerCount = 1;
        toRead.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        toRead.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,nullptr,0,nullptr,1,&toRead);

        m_context->device->endSingleTimeCommands(cmd);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_defaultWhiteImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        if (vkCreateImageView(m_context->device->getDevice(), &viewInfo, nullptr, &m_defaultWhiteImageView) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create default white texture image view!");
        }

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        if (vkCreateSampler(m_context->device->getDevice(), &samplerInfo, nullptr, &m_defaultWhiteTextureSampler) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create default white texture sampler!");
        }
        
        AE_INFO("Default white texture created successfully");
    }

    void Renderer::RendererImpl::createDepthResources() {
        auto extent = m_context->swapChain->getSwapChainExtent();
        size_t count = m_context->swapChain->getImageCount();
        m_depthImages.resize(count);
        m_depthImageAllocations.resize(count);
        m_depthImageViews.resize(count);

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = extent.width;
        imageInfo.extent.height = extent.height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = m_depthFormat;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        for (size_t i = 0; i < count; ++i) {
            if (vmaCreateImage(m_context->device->getAllocator(), &imageInfo, &allocInfo, &m_depthImages[i], &m_depthImageAllocations[i], nullptr) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create depth image!");
            }

            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = m_depthImages[i];
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = m_depthFormat;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(m_context->device->getDevice(), &viewInfo, nullptr, &m_depthImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create depth image view!");
            }
        }
    }

    void Renderer::RendererImpl::createCommandPool() {
        AstralEngine::Vulkan::QueueFamilyIndices queueFamilyIndices = m_context->device->getQueueFamilyIndices();

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        if (vkCreateCommandPool(m_context->device->getDevice(), &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS) {
            throw std::runtime_error("Komut havuzu (command pool) oluşturulamadı!");
        }
    }

    void Renderer::RendererImpl::createCommandBuffers() {
        m_commandBuffers.resize(m_context->swapChain->getImageCount());
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)m_commandBuffers.size();

        if (vkAllocateCommandBuffers(m_context->device->getDevice(), &allocInfo, m_commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("Komut tamponları (command buffers) ayrılamadı!");
        }
    }
    
    void Renderer::RendererImpl::recordCommandBuffer(uint32_t imageIndex, const ECS::Scene& scene) {
        // TODO(RenderGraph): Replace manual pass sequencing/barriers with RenderGraph once Phase 3 lands.
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(m_commandBuffers[imageIndex], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("Failed to begin recording command buffer!");
        }

        VkImageMemoryBarrier colorPre{};
        colorPre.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        colorPre.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorPre.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorPre.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        colorPre.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        colorPre.image = m_context->swapChain->getImages()[imageIndex];
        colorPre.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        colorPre.subresourceRange.baseMipLevel = 0;
        colorPre.subresourceRange.levelCount = 1;
        colorPre.subresourceRange.baseArrayLayer = 0;
        colorPre.subresourceRange.layerCount = 1;
        colorPre.srcAccessMask = 0;
        colorPre.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkImageMemoryBarrier depthPre{};
        depthPre.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        depthPre.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthPre.newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        depthPre.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        depthPre.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        depthPre.image = m_depthImages[imageIndex];
        depthPre.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        depthPre.subresourceRange.baseMipLevel = 0;
        depthPre.subresourceRange.levelCount = 1;
        depthPre.subresourceRange.baseArrayLayer = 0;
        depthPre.subresourceRange.layerCount = 1;
        depthPre.srcAccessMask = 0;
        depthPre.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        VkImageMemoryBarrier preBarriers[2] = { colorPre, depthPre };

        vkCmdPipelineBarrier(
            m_commandBuffers[imageIndex],
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            0, 0, nullptr, 0, nullptr, 2, preBarriers
        );

        VkRenderingInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.renderArea.offset = {0, 0};
        renderingInfo.renderArea.extent = m_context->swapChain->getSwapChainExtent();
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = 1;
    
        VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        VkClearValue clearDepth = { {1.0f, 0} };
    
        VkRenderingAttachmentInfo colorAttachment{};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView = m_context->swapChain->getImageViews()[imageIndex];
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue = clearColor;

        VkRenderingAttachmentInfo depthAttachment{};
        depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAttachment.imageView = m_depthImageViews[imageIndex];
        depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.clearValue = clearDepth;
    
        renderingInfo.pColorAttachments = &colorAttachment;
        renderingInfo.pDepthAttachment = &depthAttachment;

        vkCmdBeginRendering(m_commandBuffers[imageIndex], &renderingInfo);
        
        if (m_pipeline) {
            {
                PERF_TIMER("ShadowPass");
                renderShadowMaps(m_commandBuffers[imageIndex], scene);
            }
        
            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(m_context->swapChain->getSwapChainExtent().width);
            viewport.height = static_cast<float>(m_context->swapChain->getSwapChainExtent().height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(m_commandBuffers[imageIndex], 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = {0, 0};
            scissor.extent = m_context->swapChain->getSwapChainExtent();
            vkCmdSetScissor(m_commandBuffers[imageIndex], 0, 1, &scissor);

            renderECSEntities(m_commandBuffers[imageIndex], scene, imageIndex);
        }

        vkCmdEndRendering(m_commandBuffers[imageIndex]);

        VkImageMemoryBarrier postRenderBarrier{};
        postRenderBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        postRenderBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        postRenderBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        postRenderBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        postRenderBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        postRenderBarrier.image = m_context->swapChain->getImages()[imageIndex];
        postRenderBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        postRenderBarrier.subresourceRange.baseMipLevel = 0;
        postRenderBarrier.subresourceRange.levelCount = 1;
        postRenderBarrier.subresourceRange.baseArrayLayer = 0;
        postRenderBarrier.subresourceRange.layerCount = 1;
        postRenderBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        postRenderBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

        vkCmdPipelineBarrier(m_commandBuffers[imageIndex], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &postRenderBarrier);

        if (vkEndCommandBuffer(m_commandBuffers[imageIndex]) != VK_SUCCESS) {
            throw std::runtime_error("Komut tamponu kaydı bitirilemedi!");
        }
    }

    void Renderer::RendererImpl::recordCommandBuffer(uint32_t imageIndex) {
        ECS::Scene tempScene;
        recordCommandBuffer(imageIndex, tempScene);
    }
    
    void Renderer::RendererImpl::updateUniformBuffer(uint32_t currentImage, const ECS::Scene& scene) {
        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        Vulkan::UniformBufferObject ubo{};
        
        if (scene.hasMainCamera()) {
            EntityID cameraEntity = scene.getMainCamera();
            if (scene.hasComponent<ECS::Camera>(cameraEntity) && scene.hasComponent<ECS::Transform>(cameraEntity)) {
                const auto& camera = scene.getComponent<ECS::Camera>(cameraEntity);
                const auto& cameraTransform = scene.getComponent<ECS::Transform>(cameraEntity);
                
                ubo.view = camera.getViewMatrix(cameraTransform);
                ubo.proj = camera.getProjectionMatrix();
                ubo.proj[1][1] *= -1;
            } else {
                AE_WARN("Main camera entity missing required components, using identity matrices");
                ubo.view = glm::mat4(1.0f);
                ubo.proj = glm::mat4(1.0f);
            }
        } else {
            AE_WARN("No main camera set in scene, using default perspective");
            ubo.view = glm::lookAt(glm::vec3(0, 0, 3), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
            ubo.proj = glm::perspective(
                glm::radians(45.0f), 
                m_context->swapChain->getSwapChainExtent().width / (float)m_context->swapChain->getSwapChainExtent().height, 
                0.1f, 100.0f
            );
            ubo.proj[1][1] *= -1;
        }

        void* data;
        vmaMapMemory(m_context->device->getAllocator(), m_uniformBuffers[currentImage].getAllocation(), &data);
        memcpy(data, &ubo, sizeof(ubo));
        vmaUnmapMemory(m_context->device->getAllocator(), m_uniformBuffers[currentImage].getAllocation());
    }
    
    void Renderer::RendererImpl::renderECSEntities(VkCommandBuffer commandBuffer, const ECS::Scene& scene, uint32_t imageIndex) {
        PERF_TIMER("Renderer::renderECSEntities");
        STACK_SCOPE();
        
        auto* frameAllocator = AstralEngine::Memory::MemoryManager::getInstance().getFrameAllocator();
        
        const size_t maxRenderItems = 1024;
        const size_t maxRenderBatches = 64;
        
        RenderItem* renderItems = frameAllocator->allocateArray<RenderItem>(maxRenderItems);
        RenderBatch* renderBatches = frameAllocator->allocateArray<RenderBatch>(maxRenderBatches);
        
        if (!renderItems || !renderBatches) {
            AE_ERROR("Failed to allocate render arrays on frame allocator");
            return;
        }
        
        size_t renderItemCount = 0;
        size_t renderBatchCount = 0;
        
        collectRenderItems(scene, renderItems, renderItemCount, maxRenderItems);
#include "Renderer.h"
#include "Platform/Window.h"
#include "Core/Logger.h"
#include "Core/EngineConfig.h"
#include "ECS/ECS.h"
#include <stdexcept>
#include <atomic>

namespace AstralEngine {

    class Renderer::RendererImpl {
    public:
        explicit RendererImpl(Window& window) : m_window(window), m_renderSystem(window) {
            m_initialized = false;
            m_framebufferResized = false;
            m_shouldExit = false;
            
            try {
                m_initialized = m_renderSystem.initialize();
                if (m_initialized) {
                    AE_INFO("Renderer successfully initialized");
                } else {
                    AE_ERROR("Failed to initialize Renderer");
                }
            } catch (const std::exception& e) {
                AE_ERROR("Exception during Renderer initialization: {}", e.what());
                m_initialized = false;
            }
        }

        ~RendererImpl() {
            AE_INFO("Destroying Renderer");
            // Cleanup handled by RenderSystem
            AE_INFO("Renderer destroyed successfully");
        }

        void drawFrame(const ECS::Scene& scene) {
            if (!m_initialized) {
                AE_ERROR("Renderer not initialized, cannot draw frame");
                return;
            }
            
            auto& config = ENGINE_CONFIG();
            
            if (m_framebufferResized) {
                m_renderSystem.onFramebufferResize();
                m_framebufferResized = false;
                return;
            }
            
            // Begin frame
            m_renderSystem.beginFrame();
            
            // For now, we'll just log that we're drawing a frame
            // In a full implementation, we would determine whether to render 2D or 3D
            // based on the current editor mode
            AE_DEBUG("Drawing frame for scene with {} entities", scene.getEntityCount());
            
            // End frame
            m_renderSystem.endFrame();
        }

        void drawFrame() {
            ECS::Scene tempScene;
            drawFrame(tempScene);
        }

        void onFramebufferResize() {
            AE_DEBUG("Framebuffer resize requested");
            m_framebufferResized = true;
        }

        Vulkan::VulkanDevice& getDevice() {
            return m_renderSystem.getDevice();
        }

        const Vulkan::VulkanDevice& getDevice() const {
            return m_renderSystem.getDevice();
        }

        bool isInitialized() const {
            return m_initialized && m_renderSystem.isInitialized();
        }

        bool isFramebufferResized() const {
            return m_framebufferResized;
        }

        bool canRender() const {
            return !m_shouldExit && isInitialized();
        }

        size_t getSwapChainImageCount() const {
            // Return a default value for backward compatibility
            return 3;
        }

        SwapChainExtent getSwapChainExtent() const {
            // Return a default value for backward compatibility
            return SwapChainExtent{1200, 800};
        }

        void printDebugInfo() const {
            AE_INFO("=== Renderer Debug Info ===");
            AE_INFO("Initialized: {}", isInitialized());
            AE_INFO("Framebuffer Resized: {}", m_framebufferResized);
            m_renderSystem.printDebugInfo();
            AE_INFO("=== End Renderer Debug Info ===");
        }

        void logMemoryUsage() const {
            m_renderSystem.logMemoryUsage();
        }

        void logPipelineCacheStats() const {
            // Pipeline cache stats would be handled by the RenderSystem
            AE_DEBUG("Pipeline cache stats not implemented in refactored Renderer");
        }

        void resetPipelineCacheStats() {
            // Pipeline cache stats would be handled by the RenderSystem
            AE_DEBUG("Pipeline cache stats reset not implemented in refactored Renderer");
        }

        Window& m_window;
        RenderSystem m_renderSystem;
        bool m_initialized;
        bool m_framebufferResized;
        std::atomic<bool> m_shouldExit;
    };

    // Renderer implementation
    Renderer::Renderer(Window& window) 
        : m_impl(std::make_unique<RendererImpl>(window)) {}

    Renderer::~Renderer() = default;

    Renderer::Renderer(Renderer&&) noexcept = default;
    Renderer& Renderer::operator=(Renderer&&) noexcept = default;

    void Renderer::drawFrame(const ECS::Scene& scene) {
        m_impl->drawFrame(scene);
    }

    void Renderer::drawFrame() {
        m_impl->drawFrame();
    }

    void Renderer::onFramebufferResize() {
        m_impl->onFramebufferResize();
    }

    Vulkan::VulkanDevice& Renderer::getDevice() {
        return m_impl->getDevice();
    }

    const Vulkan::VulkanDevice& Renderer::getDevice() const {
        return m_impl->getDevice();
    }

    bool Renderer::isInitialized() const {
        return m_impl->isInitialized();
    }

    bool Renderer::isFramebufferResized() const {
        return m_impl->isFramebufferResized();
    }

    bool Renderer::canRender() const {
        return m_impl->canRender();
    }

    size_t Renderer::getSwapChainImageCount() const {
        return m_impl->getSwapChainImageCount();
    }

    Renderer::SwapChainExtent Renderer::getSwapChainExtent() const {
        return m_impl->getSwapChainExtent();
    }

    void Renderer::printDebugInfo() const {
        m_impl->printDebugInfo();
    }

    void Renderer::logMemoryUsage() const {
        m_impl->logMemoryUsage();
    }

    void Renderer::logPipelineCacheStats() const {
        m_impl->logPipelineCacheStats();
    }

    void Renderer::resetPipelineCacheStats() {
        m_impl->resetPipelineCacheStats();
    }

} // namespace AstralEngine
