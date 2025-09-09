#include "Renderer.h"
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
#include "ECS/ECS.h"
#include <stdexcept>
#include <vector>
#include <algorithm>
#include <chrono>
#include <SDL3/SDL_vulkan.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cassert>


// Hata ayıklama mesajları için callback fonksiyonu
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        AE_ERROR("Validation Layer: %s", pCallbackData->pMessage);
    } else {
        AE_WARN("Validation Layer: %s", pCallbackData->pMessage);
    }
    return VK_FALSE;
}

namespace AstralEngine {
    
    Vulkan::VulkanDevice& Renderer::getDevice() { 
        return *m_context->device; 
    }
    
    const Vulkan::VulkanDevice& Renderer::getDevice() const { 
        return *m_context->device; 
    }

    Renderer::Renderer(Window& window) : m_window(window) {
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

        m_window.SetRenderer(this);
        
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

    Renderer::~Renderer() {
        AE_INFO("Destroying Renderer");
        
        if (m_context && m_context->device) {
            vkDeviceWaitIdle(m_context->device->getDevice());
        }
        cleanup();
        
        AE_INFO("Renderer destroyed successfully");
    }

    void Renderer::destroyDefaultWhiteTexture() {
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
    
    void Renderer::destroyDepthResources() {
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
    
    void Renderer::cleanup() {
        AE_INFO("Cleaning up Vulkan resources...");

        if (!m_context || !m_context->device) {
            return; // Zaten temizlenmiş veya hiç başlatılmamış
        }
        
        // Wait for device to finish all operations
        VkDevice device = m_context->device->getDevice();
        vkDeviceWaitIdle(device);
        
        // 1. Shutdown and reset shadow manager first (contains VMA allocations)
        if (m_shadowManager) {
            m_shadowManager->shutdown();
            m_shadowManager.reset();
        }
        
        // 2. Destroy VMA-backed resources
        destroyDefaultWhiteTexture();
        destroyDepthResources();
        
        // 3. Destroy descriptor pools
        if (m_materialDescriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device, m_materialDescriptorPool, nullptr);
            m_materialDescriptorPool = VK_NULL_HANDLE;
        }
        
        if (m_descriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device, m_descriptorPool, nullptr);
            m_descriptorPool = VK_NULL_HANDLE;
        }
        
        // 4. Destroy descriptor set layouts
        if (m_materialSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device, m_materialSetLayout, nullptr);
            m_materialSetLayout = VK_NULL_HANDLE;
        }
        if (m_sceneSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device, m_sceneSetLayout, nullptr);
            m_sceneSetLayout = VK_NULL_HANDLE;
        }
        
        // 5. Reset managers before device destruction
        m_descriptorManager.reset();
        m_shaderManager.reset();
        
        // 6. Destroy uniform buffers (VMA-backed)
        m_uniformBuffers.clear();
        m_defaultMaterialBuffers.clear();
        
        // 7. Destroy command pool
        if (m_commandPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(device, m_commandPool, nullptr);
            m_commandPool = VK_NULL_HANDLE;
        }
        
        // 8. Reset models and pipelines
        m_defaultModel.reset();
        m_pipeline.reset();
        m_pipelineCache.clear();
        
        // 9. Destroy swapchain
        m_context->swapChain.reset();
        
        // 10. Finally reset device (destroys VMA allocator)
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

    void Renderer::initVulkan() {
        AE_INFO("Initializing Vulkan subsystem...");
        
        createInstance();
        setupDebugMessenger();
        createSurface();
        m_context->device = std::make_unique<Vulkan::VulkanDevice>(m_context->instance, m_context->surface);

        createDescriptorSetLayouts();
        
        // Initialize DescriptorSetManager
        m_descriptorManager = std::make_unique<Vulkan::DescriptorSetManager>(*m_context->device);
        
        // Initialize MaterialShaderManager
        m_shaderManager = std::make_unique<MaterialShaderManager>(*m_context->device);
        
        // Initialize ShadowMapManager
        m_shadowManager = std::make_unique<ShadowMapManager>(*m_context->device);
        if (!m_shadowManager->initialize({})) {
            AE_WARN("ShadowMapManager initialization failed; disabling shadows");
            m_shadowManager.reset();
        }
        
        m_context->swapChain = std::make_unique<Vulkan::VulkanSwapChain>(*m_context->device, m_window);

        // Depth resources
        m_depthFormat = findSupportedDepthFormat();
        createDepthResources();

        // Create shader
        m_shader = std::make_unique<Shader>(*m_context->device, "shaders/basic.vert", "shaders/basic.frag");
        
        // Create default white texture for materials
        createDefaultWhiteTexture();
        
        // Create material descriptor pool and sets
        createMaterialDescriptorPool();
        createMaterialDescriptorSets();

        // Create rendering pipeline with shader modules
        {
            std::vector<VkDescriptorSetLayout> setLayouts = { m_sceneSetLayout, m_materialSetLayout };
            m_pipeline = std::make_unique<Vulkan::VulkanPipeline>(
                *m_context->device, 
                setLayouts,
                m_context->swapChain->getSwapChainImageFormat(),
                m_depthFormat,
                *m_shader, // Derlenen SPIR-V modülleri kullanılır
                PipelineConfig::createDefault() // Explicitly use default opaque configuration
            );
        }
        createCommandPool();
        
        // Try to load a default model for testing/fallback
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
        createCommandBuffers();
    }

    void Renderer::drawFrame(const ECS::Scene& scene) {
        PERF_TIMER("Renderer::drawFrame");
        
        // Process deferred descriptor set deletions at the beginning of each frame
        if (m_descriptorManager) {
            m_descriptorManager->processDeferredDeletions();
        }
        
        if (!m_initialized) {
            AE_ERROR("Renderer not initialized, cannot draw frame");
            return;
        }
        
        auto& config = ENGINE_CONFIG();
        
        // Skip rendering if swapchain is being recreated in background
        if (!canRender()) {
            if (config.skipFramesDuringRecreation) {
                return;
            }
        }
        
        // Check if we need to recreate swapchain
        if (m_framebufferResized) {
            if (config.enableBackgroundSwapChainRecreation) {
                recreateSwapChainBackground();
            } else {
                recreateSwapChain();
            }
            m_framebufferResized = false;
            return; // Skip this frame
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

        // Reset error counter on successful operation
        m_consecutiveErrors.store(0);
        
        // Update scene-level uniforms (view/projection matrix)
        {
            PERF_TIMER("UpdateUniformBuffer");
            updateUniformBuffer(imageIndex, scene);
        }
    
        // Record command buffers for the current image with ECS entities
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
        
        // Log memory usage and pipeline cache stats periodically for debugging
        static uint32_t frameCounter = 0;
        if (++frameCounter % 120 == 0) { // Log every 2 seconds at 60 FPS
            logMemoryUsage();
            
            // Log pipeline cache statistics in debug builds
            #ifdef AE_DEBUG
            const uint32_t totalRequests = m_pipelineCacheHits.load(std::memory_order_relaxed) + 
                                          m_pipelineCacheMisses.load(std::memory_order_relaxed);
            if (totalRequests > 0) {
                logPipelineCacheStats();
            }
            #endif
        }
    }

    void Renderer::drawFrame() {
        // Create a temporary empty scene for backward compatibility
        ECS::Scene tempScene;
        drawFrame(tempScene);
    }

    void Renderer::onFramebufferResize() {
        AE_DEBUG("Framebuffer resize requested");
        m_framebufferResized = true;
    }

    void Renderer::recreateSwapChain() {
        // Wait for device to be idle before recreating swapchain
        vkDeviceWaitIdle(m_context->device->getDevice());

        // Cleanup existing swapchain resources
        cleanupSwapChain();

        // Recreate swapchain and related resources
        m_context->swapChain = std::make_unique<AstralEngine::Vulkan::VulkanSwapChain>(*m_context->device, m_window);
        
        // Recreate depth resources
        m_depthFormat = findSupportedDepthFormat();
        createDepthResources();

        // Material pool/sets yeniden (set sayısı swapchain image sayısına bağlı)
        createMaterialDescriptorPool();
        createMaterialDescriptorSets();

        // Recreate pipeline
        {
            std::vector<VkDescriptorSetLayout> setLayouts = { m_sceneSetLayout, m_materialSetLayout };
            m_pipeline = std::make_unique<AstralEngine::Vulkan::VulkanPipeline>(
                *m_context->device, 
                setLayouts,
                m_context->swapChain->getSwapChainImageFormat(),
                m_depthFormat,
                *m_shader,
                PipelineConfig::createDefault() // Explicitly use default opaque configuration
            );
        }
        
        // Recreate resources that depend on swapchain
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
        createCommandBuffers();
        
        // Mark command buffers as dirty since we recreated them
        m_commandBuffersDirty = true;
    }

    void Renderer::cleanupSwapChain() {
        // Clean up pipeline cache since pipelines are format-sensitive
        for (auto& [key, pipeline] : m_pipelineCache) {
            pipeline.reset(); // VulkanPipeline destructor releases Vulkan objects
        }
        m_pipelineCache.clear();
        AE_DEBUG("Cleared pipeline cache during swapchain cleanup");
        
        // Clean up resources that depend on swapchain
        if (m_materialDescriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(m_context->device->getDevice(), m_materialDescriptorPool, nullptr);
            m_materialDescriptorPool = VK_NULL_HANDLE;
        }
        if (m_descriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(m_context->device->getDevice(), m_descriptorPool, nullptr);
            m_descriptorPool = VK_NULL_HANDLE;
        }

        // Clear uniform buffers properly
        m_uniformBuffers.clear();
        m_defaultMaterialBuffers.clear();

        // Clean up command buffers properly
        if (!m_commandBuffers.empty() && m_commandPool != VK_NULL_HANDLE) {
            vkFreeCommandBuffers(m_context->device->getDevice(), m_commandPool, static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());
            m_commandBuffers.clear();
        }

        // Destroy depth resources
        for (auto view : m_depthImageViews) {
            vkDestroyImageView(m_context->device->getDevice(), view, nullptr);
        }
        m_depthImageViews.clear();
        for (size_t i = 0; i < m_depthImages.size(); ++i) {
            if (m_depthImages[i]) {
                vmaDestroyImage(m_context->device->getAllocator(), m_depthImages[i], m_depthImageAllocations[i]);
            }
        }
        m_depthImages.clear();
        m_depthImageAllocations.clear();

        // Reset pipeline and swapchain (they will be recreated)
        m_pipeline.reset();
        m_context->swapChain.reset();

        // Destroy default white material resources
        if (m_defaultWhiteTextureSampler) {
            vkDestroySampler(m_context->device->getDevice(), m_defaultWhiteTextureSampler, nullptr);
            m_defaultWhiteTextureSampler = VK_NULL_HANDLE;
        }
        if (m_defaultWhiteImageView) {
            vkDestroyImageView(m_context->device->getDevice(), m_defaultWhiteImageView, nullptr);
            m_defaultWhiteImageView = VK_NULL_HANDLE;
        }
        if (m_defaultWhiteImage) {
            vmaDestroyImage(m_context->device->getAllocator(), m_defaultWhiteImage, m_defaultWhiteImageAllocation);
            m_defaultWhiteImage = VK_NULL_HANDLE;
            m_defaultWhiteImageAllocation = VK_NULL_HANDLE;
        }
    }

    void Renderer::createCommandPool() {
        AstralEngine::Vulkan::QueueFamilyIndices queueFamilyIndices = m_context->device->getQueueFamilyIndices();

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        if (vkCreateCommandPool(m_context->device->getDevice(), &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS) {
            throw std::runtime_error("Komut havuzu (command pool) oluşturulamadı!");
        }
    }

    void Renderer::createCommandBuffers() {
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

    void Renderer::recordCommandBuffer(uint32_t imageIndex, const ECS::Scene& scene) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(m_commandBuffers[imageIndex], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("Failed to begin recording command buffer!");
        }

        // Transition the swap chain image to the appropriate layout for rendering
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
            0,
            0, nullptr,
            0, nullptr,
            2, preBarriers
        );

        // Use Dynamic Rendering instead of traditional RenderPass
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
        
        // Only bind pipeline and draw if we have a valid pipeline
        if (m_pipeline) {
            m_pipeline->Bind(m_commandBuffers[imageIndex]);
        
        // SHADOW PASS - Render shadow maps first
        {
            PERF_TIMER("ShadowPass");
            renderShadowMaps(m_commandBuffers[imageIndex], scene);
        }
        
        // Set viewport and scissor outside the loop as they're typically the same for all objects
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

        // MAIN PASS - Render scene with shadows
        renderECSEntities(m_commandBuffers[imageIndex], scene, imageIndex);
        }

        vkCmdEndRendering(m_commandBuffers[imageIndex]);

        // Transition the swap chain image to the present layout
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

        vkCmdPipelineBarrier(
            m_commandBuffers[imageIndex],
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &postRenderBarrier
        );

        if (vkEndCommandBuffer(m_commandBuffers[imageIndex]) != VK_SUCCESS) {
            throw std::runtime_error("Komut tamponu kaydı bitirilemedi!");
        }
    }

    void Renderer::recordCommandBuffer(uint32_t imageIndex) {
        // Create a temporary empty scene for backward compatibility
        ECS::Scene tempScene;
        recordCommandBuffer(imageIndex, tempScene);
    }
    
    void Renderer::renderECSEntities(VkCommandBuffer commandBuffer, const ECS::Scene& scene, uint32_t imageIndex) {
        PERF_TIMER("Renderer::renderECSEntities");
        STACK_SCOPE(); // Use stack allocator for this function's lifetime
        
        // Use Frame Allocator for temporary render data structures
        auto* frameAllocator = AstralEngine::Memory::MemoryManager::getInstance().getFrameAllocator();
        
        // Allocate arrays on frame allocator for maximum performance
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
        
        // Collect all renderable items
        collectRenderItems(scene, renderItems, renderItemCount, maxRenderItems);
        
        if (renderItemCount == 0) {
            AE_DEBUG("No render items collected for this frame");
            return;
        }
        
        // Create render batches grouped by material
        createRenderBatches(renderItems, renderItemCount, renderBatches, renderBatchCount, maxRenderBatches);
        
        if (renderBatchCount == 0) {
            AE_DEBUG("No render batches created");
            return;
        }
        
        // Sort batches for optimal rendering (opaque first, then transparent)
        sortRenderBatches(renderBatches, renderBatchCount);
        
        // Render all batches using the efficient batching system
        this->renderBatches(commandBuffer, renderBatches, renderBatchCount, imageIndex);
    }
    
    void Renderer::updateUniformBuffer(uint32_t currentImage, const ECS::Scene& scene) {
        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        Vulkan::UniformBufferObject ubo{};
        
        // Get camera from ECS scene
        if (scene.hasMainCamera()) {
            EntityID cameraEntity = scene.getMainCamera();
            if (scene.hasComponent<ECS::Camera>(cameraEntity) && scene.hasComponent<ECS::Transform>(cameraEntity)) {
                const auto& camera = scene.getComponent<ECS::Camera>(cameraEntity);
                const auto& cameraTransform = scene.getComponent<ECS::Transform>(cameraEntity);
                
                // Calculate view matrix from camera transform
                ubo.view = camera.getViewMatrix(cameraTransform);
                ubo.proj = camera.getProjectionMatrix();
                ubo.proj[1][1] *= -1; // Flip Y for Vulkan
            } else {
                // Fallback to identity matrices if camera components are missing
                AE_WARN("Main camera entity missing required components, using identity matrices");
                ubo.view = glm::mat4(1.0f);
                ubo.proj = glm::mat4(1.0f);
            }
        } else {
            // Fallback camera if no main camera is set
            AE_WARN("No main camera set in scene, using default perspective");
            ubo.view = glm::lookAt(glm::vec3(0, 0, 3), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
            ubo.proj = glm::perspective(
                glm::radians(45.0f), 
                m_context->swapChain->getSwapChainExtent().width / (float)m_context->swapChain->getSwapChainExtent().height, 
                0.1f, 100.0f
            );
            ubo.proj[1][1] *= -1; // Flip Y for Vulkan
        }

        void* data;
        vmaMapMemory(m_context->device->getAllocator(), m_uniformBuffers[currentImage].getAllocation(), &data);
        memcpy(data, &ubo, sizeof(ubo));
        vmaUnmapMemory(m_context->device->getAllocator(), m_uniformBuffers[currentImage].getAllocation());
    }

    void Renderer::createInstance() {
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

    void Renderer::setupDebugMessenger() {
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

    void Renderer::createSurface() {
        if (!SDL_Vulkan_CreateSurface(m_window.GetNativeWindow(), m_context->instance, nullptr, &m_context->surface)) {
            throw std::runtime_error("Vulkan yüzeyi (surface) oluşturulamadı!");
        }
        AE_INFO("Vulkan yüzeyi başarıyla oluşturuldu.");
    }

    void Renderer::createDescriptorSetLayouts() {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &uboLayoutBinding;

        if (vkCreateDescriptorSetLayout(m_context->device->getDevice(), &layoutInfo, nullptr, &m_sceneSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("Scene descriptor set layout oluşturulamadı!");
        }

        // Material set layout (set = 1)
        VkDescriptorSetLayoutBinding samplerBinding{};
        samplerBinding.binding = 0;
        samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerBinding.descriptorCount = 1;
        samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutBinding materialUboBinding{};
        materialUboBinding.binding = 1;
        materialUboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        materialUboBinding.descriptorCount = 1;
        materialUboBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        std::vector<VkDescriptorSetLayoutBinding> matBindings = { samplerBinding, materialUboBinding };

        VkDescriptorSetLayoutCreateInfo matLayoutInfo{};
        matLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        matLayoutInfo.bindingCount = static_cast<uint32_t>(matBindings.size());
        matLayoutInfo.pBindings = matBindings.data();
        if (vkCreateDescriptorSetLayout(m_context->device->getDevice(), &matLayoutInfo, nullptr, &m_materialSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("Material descriptor set layout oluşturulamadı!");
        }
    }

    void Renderer::createUniformBuffers() {
        VkDeviceSize bufferSize = sizeof(Vulkan::UniformBufferObject);
        size_t swapChainImageCount = m_context->swapChain->getImageCount();
        m_uniformBuffers.clear();
        m_uniformBuffers.reserve(swapChainImageCount);

        for (size_t i = 0; i < swapChainImageCount; i++) {
            m_uniformBuffers.emplace_back(*m_context->device, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        }
    }

    void Renderer::createDescriptorPool() {
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

    VkFormat Renderer::findSupportedDepthFormat() const {
        // Prefer D32_SFLOAT, fallback to D24_UNORM_S8_UINT if needed
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

    void Renderer::createDepthResources() {
        // Create one depth image per swapchain image
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
    
    // Statistics and utility methods
    size_t Renderer::getSwapChainImageCount() const {
        return m_context && m_context->swapChain ? m_context->swapChain->getImageCount() : 0;
    }
    
    VkExtent2D Renderer::getSwapChainExtent() const {
        return m_context && m_context->swapChain ? m_context->swapChain->getSwapChainExtent() : VkExtent2D{0, 0};
    }
    
    void Renderer::printDebugInfo() const {
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
    
	void Renderer::transitionImageLayout(VkImage image, VkFormat format, 
	                                    VkImageLayout oldLayout, VkImageLayout newLayout) {
		// Delegate to VulkanUtils for consistent behavior
		Vulkan::VulkanUtils::transitionImageLayout(*m_context->device, image, format, oldLayout, newLayout, 1);
	}

    void Renderer::createDescriptorSets() {
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

    void Renderer::createMaterialDescriptorPool() {
        // For unified material system, we need more descriptors to handle all texture slots
        std::vector<VkDescriptorPoolSize> poolSizes;
        
        // UBO for each swapchain image
        poolSizes.push_back({
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
            static_cast<uint32_t>(m_context->swapChain->getImageCount())
        });
        
        // Texture samplers - support for up to 16 texture slots per material per swapchain image
        poolSizes.push_back({
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 
            static_cast<uint32_t>(m_context->swapChain->getImageCount() * static_cast<uint32_t>(TextureSlot::Count))
        });

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; // Allow freeing individual sets
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(m_context->swapChain->getImageCount() * 100); // Support many materials
        
        if (vkCreateDescriptorPool(m_context->device->getDevice(), &poolInfo, nullptr, &m_materialDescriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create unified material descriptor pool!");
        }
        
        AE_INFO("Created unified material descriptor pool with {} texture slots", static_cast<uint32_t>(TextureSlot::Count));
    }

    void Renderer::createMaterialDescriptorSets() {
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

        // Create default unified material UBO data
        UnifiedMaterialUBO defaultMaterialData{};
        defaultMaterialData.baseColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        defaultMaterialData.metallic = 0.0f;
        defaultMaterialData.roughness = 0.5f;
        defaultMaterialData.emissiveColor = glm::vec3(0.0f, 0.0f, 0.0f);
        setTextureFlag(defaultMaterialData, TextureFlags::HasBaseColor, false);
        
        m_defaultMaterialBuffers.clear();
        m_defaultMaterialBuffers.reserve(m_context->swapChain->getImageCount());
        
        for (size_t i = 0; i < m_context->swapChain->getImageCount(); i++) {
            // Create default material UBO buffer
            m_defaultMaterialBuffers.emplace_back(
                *m_context->device, 
                sizeof(UnifiedMaterialUBO), 
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
                VMA_MEMORY_USAGE_CPU_TO_GPU
            );
            
            // Update buffer with default material data
            void* bufferData;
            vmaMapMemory(m_context->device->getAllocator(), m_defaultMaterialBuffers[i].getAllocation(), &bufferData);
            memcpy(bufferData, &defaultMaterialData, sizeof(UnifiedMaterialUBO));
            vmaUnmapMemory(m_context->device->getAllocator(), m_defaultMaterialBuffers[i].getAllocation());
            
            // Setup descriptor writes
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = m_defaultWhiteImageView;
            imageInfo.sampler = m_defaultWhiteTextureSampler;
            
            VkDescriptorBufferInfo materialBufferInfo{};
            materialBufferInfo.buffer = m_defaultMaterialBuffers[i].getBuffer();
            materialBufferInfo.offset = 0;
            materialBufferInfo.range = sizeof(UnifiedMaterialUBO);

            VkWriteDescriptorSet writes[2]{};
            
            // Texture binding (binding 0)
            writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[0].dstSet = m_materialDescriptorSets[i];
            writes[0].dstBinding = 0;
            writes[0].dstArrayElement = 0;
            writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[0].descriptorCount = 1;
            writes[0].pImageInfo = &imageInfo;
            
            // Material UBO binding (binding 1)
            writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[1].dstSet = m_materialDescriptorSets[i];
            writes[1].dstBinding = 1;
            writes[1].dstArrayElement = 0;
            writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            writes[1].descriptorCount = 1;
            writes[1].pBufferInfo = &materialBufferInfo;

            vkUpdateDescriptorSets(m_context->device->getDevice(), 2, writes, 0, nullptr);
        }
    }

    void Renderer::createDefaultWhiteTexture() {
        // Create 1x1 RGBA8 white pixel
        uint32_t pixel = 0xFFFFFFFFu;
        VkDeviceSize imageSize = sizeof(uint32_t);

        Vulkan::VulkanBuffer staging(*m_context->device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
        void* data;
        vmaMapMemory(m_context->device->getAllocator(), staging.getAllocation(), &data);
        memcpy(data, &pixel, sizeof(pixel));
        vmaUnmapMemory(m_context->device->getAllocator(), staging.getAllocation());

        // Create image
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

        // Transition to TRANSFER_DST, copy, then to SHADER_READ_ONLY
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

        // Create view
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

        // Create sampler
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
    
    bool Renderer::waitForDeviceIdle(uint32_t timeoutMs) {
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
            
            // Short sleep to prevent busy waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    
    void Renderer::recreateSwapChainBackground() {
        auto& config = ENGINE_CONFIG();
        
        // Check if already recreating
        if (m_recreatingSwapChain.load()) {
            AE_DEBUG("SwapChain already being recreated in background");
            return;
        }
        
        m_recreatingSwapChain.store(true);
        AE_INFO("Starting background SwapChain recreation");
        
        // Launch background thread for swapchain recreation
        std::thread([this, &config]() {
            try {
                PERF_TIMER("Background SwapChain Recreation");
                
                // Wait for device to be idle with timeout
                if (!waitForDeviceIdle(config.swapChainRecreationTimeoutMs)) {
                    AE_ERROR("Timeout waiting for device idle during background swapchain recreation");
                    m_recreatingSwapChain.store(false);
                    return;
                }
                
                // Cleanup existing swapchain resources
                cleanupSwapChain();
                
                // Recreate swapchain and related resources
                m_context->swapChain = std::make_unique<AstralEngine::Vulkan::VulkanSwapChain>(*m_context->device, m_window);
                
                // Recreate depth resources
                m_depthFormat = findSupportedDepthFormat();
                createDepthResources();
                
                // Material pool/sets yeniden (set sayısı swapchain image sayısına bağlı)
                createMaterialDescriptorPool();
                createMaterialDescriptorSets();
                
                // Recreate pipeline
                {
                    std::vector<VkDescriptorSetLayout> setLayouts = { m_sceneSetLayout, m_materialSetLayout };
                    m_pipeline = std::make_unique<AstralEngine::Vulkan::VulkanPipeline>(
                        *m_context->device, 
                        setLayouts,
                        m_context->swapChain->getSwapChainImageFormat(),
                        m_depthFormat,
                        *m_shader,
                        PipelineConfig::createDefault() // Explicitly use default opaque configuration
                    );
                }
                
                // Recreate resources that depend on swapchain
                createUniformBuffers();
                createDescriptorPool();
                createDescriptorSets();
                createCommandBuffers();
                
                // Mark command buffers as dirty since we recreated them
                m_commandBuffersDirty = true;
                
                AE_INFO("Background SwapChain recreation completed successfully");
                
            } catch (const std::exception& e) {
                AE_ERROR("Failed to recreate SwapChain in background: {}", e.what());
                m_consecutiveErrors++;
                
                // If too many errors, mark for exit
                if (m_consecutiveErrors.load() > ENGINE_CONFIG().maxConsecutiveRenderErrors) {
                    AE_ERROR("Too many SwapChain recreation failures, marking for exit");
                    m_shouldExit.store(true);
                }
            }
            
            m_recreatingSwapChain.store(false);
        }).detach();
    }
    
    bool Renderer::canRender() const {
        return !m_recreatingSwapChain.load() && !m_shouldExit.load() && m_initialized;
    }
    
    void Renderer::collectRenderItems(const ECS::Scene& scene, RenderItem* outItems, size_t& outCount, size_t maxItems) {
        PERF_TIMER("Renderer::collectRenderItems");
        STACK_SCOPE(); // Use stack allocator for temporary calculations
        
        outCount = 0;
        
        // Get camera position for distance calculation
        glm::vec3 cameraPos(0.0f, 0.0f, 3.0f); // Default camera position
        if (scene.hasMainCamera()) {
            EntityID cameraEntity = scene.getMainCamera();
            if (scene.hasComponent<ECS::Transform>(cameraEntity)) {
                const auto& cameraTransform = scene.getComponent<ECS::Transform>(cameraEntity);
                cameraPos = cameraTransform.position;
            }
        }
        
        // Collect all renderable entities from the scene
        auto view = scene.getRegistry().view<ECS::Transform, ECS::RenderComponent>();
        
        for (auto [entityId, transform, renderComp] : view) {
            if (outCount >= maxItems) {
                AE_WARN("Reached maximum render items limit: {}", maxItems);
                break;
            }
            
            if (!renderComp.visible || !renderComp.model || !renderComp.materialOverride) {
                continue;
            }
            
            // Calculate world matrix
            glm::mat4 worldMatrix = transform.getWorldMatrix(const_cast<ECS::ArchetypeRegistry&>(scene.getRegistry()));
            
            // Calculate distance to camera for sorting
            glm::vec3 worldPos = glm::vec3(worldMatrix[3]);
            float distance = glm::length(worldPos - cameraPos);
            
            // TODO: Add frustum culling here
            
            // Construct RenderItem directly in the allocated array
            new (&outItems[outCount]) RenderItem(entityId, renderComp.model, renderComp.materialOverride, worldMatrix, distance);
            outCount++;
        }
        
        AE_DEBUG("Collected {} render items", outCount);
    }
    
    void Renderer::createRenderBatches(const RenderItem* items, size_t itemCount, RenderBatch* outBatches, size_t& outBatchCount, size_t maxBatches) {
        PERF_TIMER("Renderer::createRenderBatches");
        STACK_SCOPE(); // Use stack allocator for material grouping
        
        outBatchCount = 0;
        
        // Group items by composite key (shader + pipeline config) for efficient batching
        std::unordered_map<PipelineCacheKey, std::vector<const RenderItem*>, PipelineCacheKeyHash> pipelineGroups;
        pipelineGroups.reserve(32); // Pre-reserve for common case
        
        for (size_t i = 0; i < itemCount; ++i) {
            const auto* item = &items[i];
            if (!item->material) continue;
            
            // Generate composite cache key
            const uint64_t shaderHash = item->material->getVariantHash();
            const PipelineConfig pipelineConfig = item->material->getPipelineConfig();
            const uint64_t configHash = static_cast<uint64_t>(std::hash<PipelineConfig>{}(pipelineConfig));
            PipelineCacheKey cacheKey{ shaderHash, configHash };
            
            pipelineGroups[cacheKey].push_back(item);
        }
        
        // Create batches from pipeline groups
        for (const auto& [cacheKey, groupItems] : pipelineGroups) {
            if (outBatchCount >= maxBatches) {
                AE_WARN("Reached maximum batch limit: {}", maxBatches);
                break;
            }
            
            if (groupItems.empty()) continue;
            
            // Initialize batch directly in allocated array
            RenderBatch& batch = outBatches[outBatchCount];
            batch = RenderBatch(groupItems[0]->material);
            
            // Ensure material has descriptor sets built
            if (batch.material) {
                // Build descriptor sets if not already built or needs rebuilding
                size_t swapChainImageCount = m_context->swapChain->getImages().size();
                if (!batch.material->isDescriptorSetBuilt() || 
                    batch.material->needsDescriptorSetRebuild(swapChainImageCount)) {
                    
                    batch.material->buildDescriptorSets(
                        *m_context->device,
                        m_materialSetLayout,
                        *m_descriptorManager,
                        m_defaultWhiteImageView,
                        m_defaultWhiteTextureSampler,
                        swapChainImageCount
                    );
                    AE_DEBUG("Built descriptor sets for material with shader hash: {}, config hash: {}", 
                            cacheKey.shaderVariantHash, cacheKey.pipelineConfigHash);
                }
                
                batch.shader = m_shaderManager->getShader(*batch.material);
                
                if (!batch.shader) {
                    AE_ERROR("Failed to get shader for material variant hash: {}", cacheKey.shaderVariantHash);
                    continue; // Skip this batch
                }
                
                // Get or create pipeline for this shader + config combination
                auto pipelineIt = m_pipelineCache.find(cacheKey);
                if (pipelineIt == m_pipelineCache.end()) {
                    // Cache miss - create new pipeline with the derived configuration
                    m_pipelineCacheMisses.fetch_add(1, std::memory_order_relaxed);
                    try {
                        const PipelineConfig pipelineConfig = batch.material->getPipelineConfig();
                        std::vector<VkDescriptorSetLayout> setLayouts = { m_sceneSetLayout, m_materialSetLayout };
                        auto pipeline = std::make_shared<Vulkan::VulkanPipeline>(
                            *m_context->device,
                            setLayouts,
                            m_context->swapChain->getSwapChainImageFormat(),
                            m_depthFormat,
                            *batch.shader,
                            pipelineConfig  // Pass the derived pipeline configuration
                        );
                        m_pipelineCache[cacheKey] = pipeline;
                        batch.pipeline = pipeline;
                        
                        AE_DEBUG("PSO cache miss. Created pipeline (shader={:#x}, state={:#x})", 
                                cacheKey.shaderVariantHash, cacheKey.pipelineConfigHash);
                    } catch (const std::exception& e) {
                        m_pipelineCreationErrors.fetch_add(1, std::memory_order_relaxed);
                        AE_ERROR("Failed to create pipeline (shader={:#x}, config={:#x}): {}", 
                                cacheKey.shaderVariantHash, cacheKey.pipelineConfigHash, e.what());
                        continue; // Skip this batch
                    }
                } else {
                    // Cache hit - reuse existing pipeline
                    m_pipelineCacheHits.fetch_add(1, std::memory_order_relaxed);
                    batch.pipeline = pipelineIt->second;
                    AE_DEBUG("PSO cache hit. Reused pipeline (shader={:#x}, state={:#x})", 
                            cacheKey.shaderVariantHash, cacheKey.pipelineConfigHash);
                }
            } else {
                AE_WARN("Batch has no material, using fallback");
                // TODO: Use a default material/pipeline
                continue;
            }
            
            // Add all items to the batch
            batch.items.clear();
            batch.items.reserve(groupItems.size());
            for (const auto& item : groupItems) {
                batch.items.push_back(const_cast<RenderItem*>(item));
            }
            
            outBatchCount++;
        }
        
        AE_DEBUG("Created {} render batches from {} items", outBatchCount, itemCount);
    }
    
    void Renderer::sortRenderBatches(RenderBatch* batches, size_t batchCount) {
        PERF_TIMER("Renderer::sortRenderBatches");
        STACK_SCOPE(); // Use stack allocator for sorting operations
        
        // Sort batches by render queue (opaque first, then alpha test, then transparent)
        std::sort(batches, batches + batchCount, 
                 [](const RenderBatch& a, const RenderBatch& b) {
                     return a.renderQueue < b.renderQueue;
                 });
        
        // Sort items within each batch
        for (size_t i = 0; i < batchCount; ++i) {
            auto& batch = batches[i];
            if (batch.material && batch.material->isTransparent()) {
                // Sort transparent objects back-to-front
                std::sort(batch.items.begin(), batch.items.end(),
                         [](const RenderItem* a, const RenderItem* b) {
                             return a->distanceToCamera > b->distanceToCamera;
                         });
            } else {
                // Sort opaque objects front-to-back
                std::sort(batch.items.begin(), batch.items.end(),
                         [](const RenderItem* a, const RenderItem* b) {
                             return a->distanceToCamera < b->distanceToCamera;
                         });
            }
        }
    }
    
    void Renderer::renderBatches(VkCommandBuffer commandBuffer, const RenderBatch* batches, size_t batchCount, uint32_t imageIndex) {
        PERF_TIMER("Renderer::renderBatches");
        STACK_SCOPE(); // Use stack allocator for render batch processing
        
        uint32_t renderedBatchCount = 0;
        uint32_t itemCount = 0;
        
        for (size_t i = 0; i < batchCount; ++i) {
            const auto& batch = batches[i];
            
            if (batch.items.empty() || !batch.pipeline || !batch.shader) {
                continue;
            }
            
            renderedBatchCount++;
            itemCount += batch.items.size();
            
            // Bind pipeline for this material variant
            batch.pipeline->Bind(commandBuffer);
            
            // Debug validation: ensure pipeline matches material state
            #ifdef AE_DEBUG
            if (batch.material && batch.pipeline) {
                const PipelineConfig expectedConfig = batch.material->getPipelineConfig();
                const uint64_t expectedConfigHash = static_cast<uint64_t>(std::hash<PipelineConfig>{}(expectedConfig));
                const uint64_t expectedShaderHash = batch.material->getVariantHash();
                
                // Create expected cache key
                PipelineCacheKey expectedKey{ expectedShaderHash, expectedConfigHash };
                
                // Find the pipeline in cache and validate it matches
                auto it = m_pipelineCache.find(expectedKey);
                if (it != m_pipelineCache.end() && it->second.get() == batch.pipeline.get()) {
                    // Pipeline matches - validation passed
                } else {
                    AE_ERROR("Pipeline-material state mismatch! Expected shader={:#x}, config={:#x}", 
                            expectedShaderHash, expectedConfigHash);
                    // In debug builds, this indicates a bug in pipeline caching logic
                    assert(false && "Pipeline does not match material state");
                }
            }
            #endif
            
            // Bind scene descriptor set (set 0)
            if (imageIndex < m_descriptorSets.size()) {
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                      batch.pipeline->getPipelineLayout(), 0, 1, 
                                      &m_descriptorSets[imageIndex], 0, nullptr);
            }
            
            // Bind material-specific descriptor set (set 1)
            if (batch.material && batch.material->isDescriptorSetBuilt()) {
                VkDescriptorSet materialDescSet = batch.material->getDescriptorSet(imageIndex);
                if (materialDescSet != VK_NULL_HANDLE) {
                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                          batch.pipeline->getPipelineLayout(), 1, 1, 
                                          &materialDescSet, 0, nullptr);
                }
            } else if (imageIndex < m_materialDescriptorSets.size()) {
                // Fallback to default material descriptor set
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                      batch.pipeline->getPipelineLayout(), 1, 1, 
                                      &m_materialDescriptorSets[imageIndex], 0, nullptr);
            }
            
            // Render all items in this batch
            for (const auto* item : batch.items) {
                if (!item->model) {
                    AE_WARN("RenderItem has no model, skipping");
                    continue;
                }
                
                // Push world matrix as push constant
                vkCmdPushConstants(
                    commandBuffer,
                    batch.pipeline->getPipelineLayout(),
                    VK_SHADER_STAGE_VERTEX_BIT,
                    0,
                    sizeof(glm::mat4),
                    &item->worldMatrix
                );
                
                // Bind vertex and index buffers
                item->model->Bind(commandBuffer);
                
                // Draw the model
                item->model->Draw(commandBuffer);
            }
        }
        
        AE_DEBUG("Rendered {} batches with {} total items", renderedBatchCount, itemCount);
    }
    
    void Renderer::logMemoryUsage() const {
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
    
    void Renderer::renderShadowMaps(VkCommandBuffer commandBuffer, const ECS::Scene& scene) {
        PERF_TIMER("Renderer::renderShadowMaps");
        STACK_SCOPE();
        
        if (!m_shadowManager) {
            return; // Shadow mapping disabled
        }
        
        // Find directional lights in the scene that cast shadows
        DirectionalLightShadow shadowData;
        bool hasDirectionalLight = false;
        
        // Look for directional lights
        auto lightView = scene.getRegistry().view<ECS::Light>();
        for (auto [entity, light] : lightView) {
            if (light.type == ECS::Light::Type::Directional && light.castShadows) {
                // Get ECS camera for shadow cascade calculation
                if (scene.hasMainCamera()) {
                    EntityID cameraEntity = scene.getMainCamera();
                    if (scene.hasComponent<ECS::Camera>(cameraEntity) && scene.hasComponent<ECS::Transform>(cameraEntity)) {
                        const auto& camera = scene.getComponent<ECS::Camera>(cameraEntity);
                        const auto& cameraTransform = scene.getComponent<ECS::Transform>(cameraEntity);
                        
                        // Calculate shadow cascades for this directional light
                        m_shadowManager->calculateDirectionalLightCascades(light, camera, cameraTransform, shadowData);
                        hasDirectionalLight = true;
                        break; // For now, only handle first directional light
                    }
                }
            }
        }
        
        if (!hasDirectionalLight) {
            // Create a default directional light for shadow casting
            ECS::Light defaultLight;
            defaultLight.type = ECS::Light::Type::Directional;
            defaultLight.direction = glm::normalize(glm::vec3(-1.0f, -1.0f, -0.5f));
            defaultLight.castShadows = true;
            defaultLight.intensity = 1.0f;
            defaultLight.color = glm::vec3(1.0f);
            
            // Use ECS camera or create a default one for shadow calculation
            if (scene.hasMainCamera()) {
                EntityID cameraEntity = scene.getMainCamera();
                if (scene.hasComponent<ECS::Camera>(cameraEntity) && scene.hasComponent<ECS::Transform>(cameraEntity)) {
                    const auto& camera = scene.getComponent<ECS::Camera>(cameraEntity);
                    const auto& cameraTransform = scene.getComponent<ECS::Transform>(cameraEntity);
                    m_shadowManager->calculateDirectionalLightCascades(defaultLight, camera, cameraTransform, shadowData);
                } else {
                    // Create temporary camera for shadow calculation
                    ECS::Camera tempCamera(45.0f, 16.0f/9.0f, 0.1f, 100.0f);
                    ECS::Transform tempTransform;
                    tempTransform.position = glm::vec3(0, 0, 3);
                    m_shadowManager->calculateDirectionalLightCascades(defaultLight, tempCamera, tempTransform, shadowData);
                }
            } else {
                // Create temporary camera for shadow calculation
                ECS::Camera tempCamera(45.0f, 16.0f/9.0f, 0.1f, 100.0f);
                ECS::Transform tempTransform;
                tempTransform.position = glm::vec3(0, 0, 3);
                m_shadowManager->calculateDirectionalLightCascades(defaultLight, tempCamera, tempTransform, shadowData);
            }
            AE_DEBUG("Using default directional light for shadows");
        }
        
        // Update shadow UBO with new data
        m_shadowManager->updateShadowUBO(shadowData);
        
        // Render each cascade
        for (uint32_t cascade = 0; cascade < shadowData.activeCascadeCount; ++cascade) {
            m_shadowManager->beginShadowPass(commandBuffer, cascade);
            
            // Bind the shadow-specific depth-only pipeline
            m_shadowManager->bindShadowPipeline(commandBuffer);
            
            // Render shadow casters using the optimized depth-only pipeline
            auto renderView = scene.getRegistry().view<ECS::Transform, ECS::RenderComponent>();
            for (auto [entityId, transform, renderComp] : renderView) {
                if (!renderComp.visible || !renderComp.model || !renderComp.castShadows) {
                    continue;
                }
                
                // Calculate world matrix for this entity
                glm::mat4 worldMatrix = transform.getWorldMatrix(const_cast<ECS::ArchetypeRegistry&>(scene.getRegistry()));
                
                // Shadow shader expects: light view-proj matrix + model matrix
                struct ShadowPushConstants {
                    glm::mat4 lightViewProj;
                    glm::mat4 model;
                } pushConstants;
                
                pushConstants.lightViewProj = shadowData.cascades[cascade].viewProjMatrix;
                pushConstants.model = worldMatrix;
                
                // Push both matrices to the shadow pipeline
                vkCmdPushConstants(
                    commandBuffer,
                    m_shadowManager->getShadowPipelineLayout(),
                    VK_SHADER_STAGE_VERTEX_BIT,
                    0,
                    sizeof(ShadowPushConstants),
                    &pushConstants
                );
                
                // Render model geometry for shadow (depth-only)
                renderComp.model->Bind(commandBuffer);
                renderComp.model->Draw(commandBuffer);
            }
            
            m_shadowManager->endShadowPass(commandBuffer);
        }
        
        AE_DEBUG("Rendered {} shadow cascades", shadowData.activeCascadeCount);
    }
    
    void Renderer::logPipelineCacheStats() const {
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
    
    void Renderer::resetPipelineCacheStats() {
        m_pipelineCacheHits.store(0, std::memory_order_relaxed);
        m_pipelineCacheMisses.store(0, std::memory_order_relaxed);
        m_pipelineCreationErrors.store(0, std::memory_order_relaxed);
        AE_DEBUG("Pipeline cache statistics reset");
    }

} // namespace AstralEngine
