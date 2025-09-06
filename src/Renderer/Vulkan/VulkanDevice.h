#pragma once

#include "VulkanContext.h"

namespace Astral::Vulkan {
    // Cihazın ihtiyaç duyduğu kuyruk ailelerinin indekslerini tutan yapı
    struct QueueFamilyIndices {
        uint32_t graphicsFamily;
        uint32_t presentFamily;
        bool graphicsFamilyHasValue = false;
        bool presentFamilyHasValue = false;
        bool isComplete() { return graphicsFamilyHasValue && presentFamilyHasValue; }
    };

    class VulkanDevice {
    public:
        VulkanDevice(VkInstance instance, VkSurfaceKHR surface);
        ~VulkanDevice();

        VulkanDevice(const VulkanDevice&) = delete;
        VulkanDevice& operator=(const VulkanDevice&) = delete;

        VkDevice GetDevice() const { return m_device; }
        VkPhysicalDevice GetPhysicalDevice() const { return m_physicalDevice; }
        VkQueue GetGraphicsQueue() const { return m_graphicsQueue; }
        VkQueue GetPresentQueue() const { return m_presentQueue; }
        QueueFamilyIndices GetQueueFamilyIndices() const { return m_indices; }
        VkSurfaceKHR GetSurface() const { return m_surface; }

        void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    private:
        void pickPhysicalDevice();
        void createLogicalDevice();

        // Helper fonksiyonlar
        bool isDeviceSuitable(VkPhysicalDevice device);
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

        VkInstance m_instance;
        VkSurfaceKHR m_surface;

        VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
        VkDevice m_device;
        QueueFamilyIndices m_indices;

        VkQueue m_graphicsQueue;
        VkQueue m_presentQueue;
        VkCommandPool m_commandPool; // Cihaza özel bir command pool
    };
}
