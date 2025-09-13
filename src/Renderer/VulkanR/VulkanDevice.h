#pragma once

#include "VulkanPhysicalDevice.h"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

// Forward declaration for VmaAllocator
struct VmaAllocator_T;
typedef struct VmaAllocator_T* VmaAllocator;

namespace AstralEngine::VulkanR {

    class VulkanDevice {
    public:
        struct DeviceCreateInfo {
            VkPhysicalDevice physicalDevice;
            const QueueFamilyIndices& queueFamilyIndices;
            const std::vector<const char*>& requiredExtensions;
            const VkPhysicalDeviceFeatures& requiredFeatures;
            bool enableValidationLayers;
        };

        VulkanDevice(const DeviceCreateInfo& createInfo);
        ~VulkanDevice();

        // Non-copyable
        VulkanDevice(const VulkanDevice&) = delete;
        VulkanDevice& operator=(const VulkanDevice&) = delete;

        VkDevice getDevice() const { return m_device; }
        VkPhysicalDevice getPhysicalDevice() const { return m_physicalDevice; }
        VkQueue getGraphicsQueue() const { return m_graphicsQueue; }
        VkQueue getPresentQueue() const { return m_presentQueue; }
        uint32_t getGraphicsQueueFamilyIndex() const { return m_queueFamilyIndices.graphicsFamily; }
        uint32_t getPresentQueueFamilyIndex() const { return m_queueFamilyIndices.presentFamily; }
        
        VkCommandPool getCommandPool() const { return m_commandPool; }
        VmaAllocator getAllocator() const { return m_allocator; }

        // Helper functions for command buffer management
        VkCommandBuffer beginSingleTimeCommands();
        void endSingleTimeCommands(VkCommandBuffer commandBuffer);

        // Buffer and image operations
        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
        void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    private:
        void createLogicalDevice(const DeviceCreateInfo& createInfo);
        void createCommandPool();
        void createAllocator();

        VkPhysicalDevice m_physicalDevice;
        VkDevice m_device;
        VkQueue m_graphicsQueue;
        VkQueue m_presentQueue;
        QueueFamilyIndices m_queueFamilyIndices;
        VkCommandPool m_commandPool;
        
        // VMA allocator
        VmaAllocator m_allocator;
    };

}