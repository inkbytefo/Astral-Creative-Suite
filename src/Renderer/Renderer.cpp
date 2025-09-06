#include "Renderer.h"
#include "Core/Logger.h"
#include "Vulkan/VulkanDevice.h" // Eksik başlık dosyası eklendi
#include "Vulkan/VulkanSwapChain.h"
#include "Renderer/Model.h"
#include <stdexcept>
#include <vector>
#include <SDL3/SDL_vulkan.h>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>

// Hata ayıklama mesajları için callback fonksiyonu
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        ASTRAL_LOG_ERROR("Validation Layer: %s", pCallbackData->pMessage);
    } else {
        ASTRAL_LOG_WARN("Validation Layer: %s", pCallbackData->pMessage);
    }
    return VK_FALSE;
}

namespace Astral {
    Renderer::Renderer(Window& window) : m_window(window) {
        try {
            InitVulkan();
        } catch (const std::exception& e) {
            ASTRAL_LOG_ERROR("Renderer başlatılırken hata oluştu: {}", e.what());
            Cleanup();
            throw;
        }
    }

    Renderer::~Renderer() {
        Cleanup();
    }

    void Renderer::InitVulkan() {
        ASTRAL_LOG_INFO("Vulkan başlatılıyor...");
        
        createInstance();
        setupDebugMessenger();
        createSurface();
        m_context.device = std::make_unique<Vulkan::VulkanDevice>(m_context.instance, m_context.surface);

        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;
        allocatorInfo.physicalDevice = m_context.device->GetPhysicalDevice(); // DÜZELTME: . -> ->
        allocatorInfo.device = m_context.device->GetDevice();                 // DÜZELTME: . -> ->
        allocatorInfo.instance = m_context.instance;
        vmaCreateAllocator(&allocatorInfo, &m_context.allocator);

        createDescriptorSetLayout();
        m_context.swapChain = std::make_unique<Vulkan::VulkanSwapChain>(*m_context.device, m_window);
        m_pipeline = std::make_unique<Vulkan::VulkanPipeline>(
            *m_context.device, 
            m_context.swapChain->GetRenderPass(),
            m_descriptorSetLayout,
            "shaders/triangle.vert.spv", 
            "shaders/triangle.frag.spv"
        );
        createCommandPool();
        m_model = std::make_unique<Model>(*m_context.device, m_context.allocator, "assets/models/cube.obj");
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
        createCommandBuffers();
    }

    void Renderer::Cleanup() {
        if (m_context.device) {
            vkDeviceWaitIdle(m_context.device->GetDevice()); // DÜZELTME: . -> ->
        }

        ASTRAL_LOG_INFO("Vulkan kaynakları temizleniyor...");
        
        vkDestroyDescriptorPool(m_context.device->GetDevice(), m_descriptorPool, nullptr); // DÜZELTME: . -> ->

        for (size_t i = 0; i < m_context.swapChain->GetImageCount(); i++) {
            vmaDestroyBuffer(m_context.allocator, m_uniformBuffers[i].buffer, m_uniformBuffers[i].allocation);
        }

        vkDestroyDescriptorSetLayout(m_context.device->GetDevice(), m_descriptorSetLayout, nullptr); // DÜZELTME: . -> ->

        if (m_commandPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(m_context.device->GetDevice(), m_commandPool, nullptr); // DÜZELTME: . -> ->
        }

        m_model.reset();
        m_pipeline.reset();
        m_context.swapChain.reset();

        if (m_context.allocator) {
            vmaDestroyAllocator(m_context.allocator);
        }

        m_context.device.reset();

        if (m_context.debugMessenger != VK_NULL_HANDLE) {
            auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(m_context.instance, "vkDestroyDebugUtilsMessengerEXT");
            if (func != nullptr) {
                func(m_context.instance, m_context.debugMessenger, nullptr);
            }
        }

        if (m_context.surface != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(m_context.instance, m_context.surface, nullptr);
        }
        if (m_context.instance != VK_NULL_HANDLE) {
            vkDestroyInstance(m_context.instance, nullptr);
        }
    }

    void Renderer::DrawFrame() {
        uint32_t imageIndex;
        auto result = m_context.swapChain->AcquireNextImage(&imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("Swap chain'den görüntü alınamadı!");
        }

        updateUniformBuffer(imageIndex);
        recordCommandBuffer(imageIndex);

        result = m_context.swapChain->SubmitCommandBuffers(&m_commandBuffers[imageIndex], &imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            // Swapchain yeniden oluşturulmalı
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("Komut tamponu gönderilemedi!");
        }
    }

    void Renderer::createCommandPool() {
        Vulkan::QueueFamilyIndices queueFamilyIndices = m_context.device->GetQueueFamilyIndices(); // DÜZELTME: . -> ->

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        if (vkCreateCommandPool(m_context.device->GetDevice(), &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS) { // DÜZELTME: . -> ->
            throw std::runtime_error("Komut havuzu (command pool) oluşturulamadı!");
        }
    }

    void Renderer::createCommandBuffers() {
        m_commandBuffers.resize(m_context.swapChain->GetImageCount());
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)m_commandBuffers.size();

        if (vkAllocateCommandBuffers(m_context.device->GetDevice(), &allocInfo, m_commandBuffers.data()) != VK_SUCCESS) { // DÜZELTME: . -> ->
            throw std::runtime_error("Komut tamponları (command buffers) ayrılamadı!");
        }
    }

    void Renderer::recordCommandBuffer(uint32_t imageIndex) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(m_commandBuffers[imageIndex], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("Komut tamponu kaydı başlatılamadı!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_context.swapChain->GetRenderPass();
        renderPassInfo.framebuffer = m_context.swapChain->GetFrameBuffer(imageIndex);
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = m_context.swapChain->GetSwapChainExtent();

        VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(m_commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        
        m_pipeline->Bind(m_commandBuffers[imageIndex]);
        
        vkCmdBindDescriptorSets(m_commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->GetPipelineLayout(), 0, 1, &m_descriptorSets[imageIndex], 0, nullptr);

        m_model->Bind(m_commandBuffers[imageIndex]);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(m_context.swapChain->GetSwapChainExtent().width);
        viewport.height = static_cast<float>(m_context.swapChain->GetSwapChainExtent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(m_commandBuffers[imageIndex], 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = m_context.swapChain->GetSwapChainExtent();
        vkCmdSetScissor(m_commandBuffers[imageIndex], 0, 1, &scissor);

        vkCmdDrawIndexed(m_commandBuffers[imageIndex], m_model->GetIndexCount(), 1, 0, 0, 0);

        vkCmdEndRenderPass(m_commandBuffers[imageIndex]);

        if (vkEndCommandBuffer(m_commandBuffers[imageIndex]) != VK_SUCCESS) {
            throw std::runtime_error("Komut tamponu kaydı bitirilemedi!");
        }
    }
    
    void Renderer::createInstance() {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Astral Engine";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "Astral Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        // --- DÜZELTME BAŞLANGICI ---
        // Gerekli instance uzantılarını SDL'den al (SDL3 Yöntemi)
        unsigned int count = 0;
        auto extensions_c = SDL_Vulkan_GetInstanceExtensions(&count); // 'const char**' yerine 'auto' kullan
        if (extensions_c == nullptr) {
            throw std::runtime_error("SDL Vulkan eklentileri alınamadı!");
        }
        std::vector<const char*> extensions(extensions_c, extensions_c + count);
        // --- DÜZELTME SONU ---
        
        // Debug messenger uzantısını ekle
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        // Validation Layers'ı etkinleştir
        const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        if (vkCreateInstance(&createInfo, nullptr, &m_context.instance) != VK_SUCCESS) {
            throw std::runtime_error("Vulkan instance oluşturulamadı!");
        }
        ASTRAL_LOG_INFO("Vulkan instance başarıyla oluşturuldu.");
    }

    void Renderer::setupDebugMessenger() {
        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;

        auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(m_context.instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            if (func(m_context.instance, &createInfo, nullptr, &m_context.debugMessenger) != VK_SUCCESS) {
                throw std::runtime_error("Debug messenger kurulamadı!");
            }
            ASTRAL_LOG_INFO("Vulkan debug messenger başarıyla kuruldu.");
        } else {
            throw std::runtime_error("Uzantı bulunamadığı için debug messenger kurulamadı!");
        }
    }

    void Renderer::createSurface() {
        if (!SDL_Vulkan_CreateSurface(m_window.GetNativeWindow(), m_context.instance, nullptr, &m_context.surface)) { // DÜZELTME: SDL3 imzası
            throw std::runtime_error("Vulkan yüzeyi (surface) oluşturulamadı!");
        }
        ASTRAL_LOG_INFO("Vulkan yüzeyi başarıyla oluşturuldu.");
    }

    void Renderer::updateUniformBuffer(uint32_t currentImage) {
        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        Vulkan::UniformBufferObject ubo{};
        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = m_camera.GetViewMatrix();
        ubo.proj = glm::perspective(glm::radians(45.0f), m_context.swapChain->GetSwapChainExtent().width / (float) m_context.swapChain->GetSwapChainExtent().height, 0.1f, 10.0f);
        ubo.proj[1][1] *= -1; // GLM'in Y eksenini Vulkan için çevir

        void* data;
        vmaMapMemory(m_context.allocator, m_uniformBuffers[currentImage].allocation, &data);
        memcpy(data, &ubo, sizeof(ubo));
        vmaUnmapMemory(m_context.allocator, m_uniformBuffers[currentImage].allocation);
    }

    void Renderer::createDescriptorSetLayout() {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &uboLayoutBinding;

        if (vkCreateDescriptorSetLayout(m_context.device->GetDevice(), &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS) { // DÜZELTME: . -> ->
            throw std::runtime_error("Descriptor set layout oluşturulamadı!");
        }
    }

    void Renderer::createUniformBuffers() {
        VkDeviceSize bufferSize = sizeof(Vulkan::UniformBufferObject);
        size_t swapChainImageCount = m_context.swapChain->GetImageCount();
        m_uniformBuffers.resize(swapChainImageCount);

        for (size_t i = 0; i < swapChainImageCount; i++) {
            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = bufferSize;
            bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

            VmaAllocationCreateInfo allocInfo{};
            allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU; 

            vmaCreateBuffer(m_context.allocator, &bufferInfo, &allocInfo, &m_uniformBuffers[i].buffer, &m_uniformBuffers[i].allocation, nullptr);
        }
    }

    void Renderer::createDescriptorPool() {
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = static_cast<uint32_t>(m_context.swapChain->GetImageCount());

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = static_cast<uint32_t>(m_context.swapChain->GetImageCount());

        if (vkCreateDescriptorPool(m_context.device->GetDevice(), &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) { // DÜZELTME: . -> ->
            throw std::runtime_error("Descriptor pool oluşturulamadı!");
        }
    }

    void Renderer::createDescriptorSets() {
        std::vector<VkDescriptorSetLayout> layouts(m_context.swapChain->GetImageCount(), m_descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(m_context.swapChain->GetImageCount());
        allocInfo.pSetLayouts = layouts.data();

        m_descriptorSets.resize(m_context.swapChain->GetImageCount());
        if (vkAllocateDescriptorSets(m_context.device->GetDevice(), &allocInfo, m_descriptorSets.data()) != VK_SUCCESS) { // DÜZELTME: . -> ->
            throw std::runtime_error("Descriptor set'ler ayrılamadı!");
        }

        for (size_t i = 0; i < m_context.swapChain->GetImageCount(); i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = m_uniformBuffers[i].buffer;
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

            vkUpdateDescriptorSets(m_context.device->GetDevice(), 1, &descriptorWrite, 0, nullptr); // DÜZELTME: . -> ->
        }
    }
}
