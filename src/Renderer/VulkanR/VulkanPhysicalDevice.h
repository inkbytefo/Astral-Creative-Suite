#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

namespace AstralEngine::VulkanR {

    struct QueueFamilyIndices {
        uint32_t graphicsFamily;
        uint32_t presentFamily;
        bool graphicsFamilyHasValue = false;
        bool presentFamilyHasValue = false;

        bool isComplete() const {
            return graphicsFamilyHasValue && presentFamilyHasValue;
        }
    };

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    class VulkanPhysicalDevice {
    public:
        struct DeviceRequirements {
            VkPhysicalDeviceFeatures requiredFeatures;
            std::vector<const char*> requiredExtensions;
            bool requireGraphicsQueue = true;
            bool requirePresentQueue = true;
        };

        VulkanPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
        ~VulkanPhysicalDevice() = default;

        // Non-copyable
        VulkanPhysicalDevice(const VulkanPhysicalDevice&) = delete;
        VulkanPhysicalDevice& operator=(const VulkanPhysicalDevice&) = delete;

        bool isDeviceSuitable(VkPhysicalDevice device, const DeviceRequirements& requirements) const;
        int rateDeviceSuitability(VkPhysicalDevice device) const;
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const;
        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) const;
        bool checkDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*>& requiredExtensions) const;

        VkPhysicalDevice getBestDevice(const DeviceRequirements& requirements) const;
        VkPhysicalDevice getPhysicalDevice() const { return m_physicalDevice; }
        const QueueFamilyIndices& getQueueFamilyIndices() const { return m_indices; }
        const SwapChainSupportDetails& getSwapChainSupport() const { return m_swapChainSupport; }

    private:
        VkInstance m_instance;
        VkSurfaceKHR m_surface;
        VkPhysicalDevice m_physicalDevice;
        QueueFamilyIndices m_indices;
        SwapChainSupportDetails m_swapChainSupport;
    };

}