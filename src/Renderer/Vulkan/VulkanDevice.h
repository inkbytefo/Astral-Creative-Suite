#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <memory>

namespace AstralEngine::Vulkan {
    struct QueueFamilyIndices {
        uint32_t graphicsFamily;
        uint32_t presentFamily;
        bool graphicsFamilyHasValue = false;
        bool presentFamilyHasValue = false;

        bool isComplete() {
            return graphicsFamilyHasValue && presentFamilyHasValue;
        }
    };

    class VulkanDevice {
    public:
        VulkanDevice(VkInstance instance, VkSurfaceKHR surface);
        ~VulkanDevice();

        // Getters
        VkPhysicalDevice getPhysicalDevice() const { return m_physicalDevice; }
        VkDevice getDevice() const { return m_device; }
        VkSurfaceKHR getSurface() const { return m_surface; }
        QueueFamilyIndices getQueueFamilyIndices() const { return m_indices; }
        VkQueue getGraphicsQueue() const { return m_graphicsQueue; }
        VkQueue getPresentQueue() const { return m_presentQueue; }
        VkCommandPool getCommandPool() const { return m_commandPool; }
        
        // --- VMA ALLOCATOR GETTER EKLEYİN ---
        VmaAllocator getAllocator() const { return m_allocator; }
        // --- VMA ALLOCATOR GETTER EKLEME SONU ---

        // Helper functions
        VkCommandBuffer beginSingleTimeCommands();
        void endSingleTimeCommands(VkCommandBuffer commandBuffer);
        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
        void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    private:
        void pickPhysicalDevice();
        void createLogicalDevice();
        void createCommandPool();
        bool isDeviceSuitable(VkPhysicalDevice device);
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

        VkInstance m_instance;
        VkSurfaceKHR m_surface;
        VkPhysicalDevice m_physicalDevice;
        VkDevice m_device;
        VkQueue m_graphicsQueue;
        VkQueue m_presentQueue;
        VkCommandPool m_commandPool;
        QueueFamilyIndices m_indices;
        
        // --- VMA ALLOCATOR ÜYE DEĞİŞKENİ ---
        VmaAllocator m_allocator;
        // --- VMA ALLOCATOR ÜYE DEĞİŞKENİ EKLEME SONU ---
    };
}