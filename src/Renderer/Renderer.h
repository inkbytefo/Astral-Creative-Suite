#pragma once

#include "Vulkan/VulkanContext.h"
#include "Platform/Window.h"
#include "Vulkan/VulkanPipeline.h" 
#include "Renderer/Model.h"
#include "Renderer/Camera.h"
#include <memory> 

namespace Astral {
    class Renderer {
    public:
        Renderer(Window& window);
        ~Renderer();

        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;

        void DrawFrame();

    private:
        void InitVulkan();
        void Cleanup();
        void createCommandPool();
        void createCommandBuffers();
        void recordCommandBuffer(uint32_t imageIndex);
        void updateUniformBuffer(uint32_t currentImage);
        void createDescriptorSetLayout();
        void createUniformBuffers();
        void createDescriptorPool();
        void createDescriptorSets();

        void createInstance();
        void setupDebugMessenger();
        void createSurface();

        Window& m_window;
        Vulkan::VulkanContext m_context;
        
        std::unique_ptr<Vulkan::VulkanPipeline> m_pipeline;
        std::unique_ptr<Astral::Model> m_model;
        Camera m_camera;

        std::vector<Vulkan::VulkanBuffer> m_uniformBuffers;
        
        VkDescriptorSetLayout m_descriptorSetLayout;
        VkDescriptorPool m_descriptorPool;
        std::vector<VkDescriptorSet> m_descriptorSets;
        VkCommandPool m_commandPool;
        std::vector<VkCommandBuffer> m_commandBuffers;
    };
}
