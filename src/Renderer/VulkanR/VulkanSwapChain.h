#pragma once

#include "VulkanDevice.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <optional>

namespace AstralEngine::VulkanR {

    class VulkanSwapChain {
    public:
        struct SwapChainCreateInfo {
            VkSurfaceKHR surface;
            uint32_t windowWidth;
            uint32_t windowHeight;
            const SwapChainSupportDetails& swapChainSupport;
            const QueueFamilyIndices& queueFamilyIndices;
        };

        VulkanSwapChain(VulkanDevice& device, const SwapChainCreateInfo& createInfo);
        ~VulkanSwapChain();

        // Non-copyable
        VulkanSwapChain(const VulkanSwapChain&) = delete;
        VulkanSwapChain& operator=(const VulkanSwapChain&) = delete;

        VkSwapchainKHR getSwapChain() const { return m_swapChain; }
        VkFormat getImageFormat() const { return m_swapChainImageFormat; }
        VkExtent2D getExtent() const { return m_swapChainExtent; }
        const std::vector<VkImage>& getImages() const { return m_swapChainImages; }
        const std::vector<VkImageView>& getImageViews() const { return m_swapChainImageViews; }

        uint32_t getImageCount() const { return static_cast<uint32_t>(m_swapChainImages.size()); }

        VkResult acquireNextImage(uint32_t* imageIndex, VkSemaphore imageAvailableSemaphore);
        VkResult presentImage(uint32_t imageIndex, VkSemaphore waitSemaphore);

        void recreateSwapChain(uint32_t windowWidth, uint32_t windowHeight);

    private:
        void createSwapChain(const SwapChainCreateInfo& createInfo);
        void createImageViews();
        void cleanupSwapChain();

        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const;
        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) const;
        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t windowWidth, uint32_t windowHeight) const;

        VulkanDevice& m_device;
        VkSwapchainKHR m_swapChain;
        VkFormat m_swapChainImageFormat;
        VkExtent2D m_swapChainExtent;

        std::vector<VkImage> m_swapChainImages;
        std::vector<VkImageView> m_swapChainImageViews;
    };

}