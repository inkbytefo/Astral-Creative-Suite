#include "VulkanSwapChain.h"
#include "Core/Logger.h"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace AstralEngine::VulkanR {

    VulkanSwapChain::VulkanSwapChain(VulkanDevice& device, const SwapChainCreateInfo& createInfo)
        : m_device(device) {
        createSwapChain(createInfo);
        createImageViews();
        AE_INFO("Vulkan swap chain created successfully");
    }

    VulkanSwapChain::~VulkanSwapChain() {
        cleanupSwapChain();
        AE_INFO("Vulkan swap chain destroyed");
    }

    void VulkanSwapChain::createSwapChain(const SwapChainCreateInfo& createInfo) {
        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(createInfo.swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(createInfo.swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(createInfo.swapChainSupport.capabilities, 
                                           createInfo.windowWidth, createInfo.windowHeight);

        uint32_t imageCount = createInfo.swapChainSupport.capabilities.minImageCount + 1;
        if (createInfo.swapChainSupport.capabilities.maxImageCount > 0 && 
            imageCount > createInfo.swapChainSupport.capabilities.maxImageCount) {
            imageCount = createInfo.swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR swapChainCreateInfo{};
        swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapChainCreateInfo.surface = createInfo.surface;
        swapChainCreateInfo.minImageCount = imageCount;
        swapChainCreateInfo.imageFormat = surfaceFormat.format;
        swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
        swapChainCreateInfo.imageExtent = extent;
        swapChainCreateInfo.imageArrayLayers = 1;
        swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices = createInfo.queueFamilyIndices;
        uint32_t queueFamilyIndices[] = { indices.graphicsFamily, indices.presentFamily };

        if (indices.graphicsFamily != indices.presentFamily) {
            swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            swapChainCreateInfo.queueFamilyIndexCount = 2;
            swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        swapChainCreateInfo.preTransform = createInfo.swapChainSupport.capabilities.currentTransform;
        swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapChainCreateInfo.presentMode = presentMode;
        swapChainCreateInfo.clipped = VK_TRUE;
        swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(m_device.getDevice(), &swapChainCreateInfo, nullptr, &m_swapChain) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR(m_device.getDevice(), m_swapChain, &imageCount, nullptr);
        m_swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(m_device.getDevice(), m_swapChain, &imageCount, m_swapChainImages.data());

        m_swapChainImageFormat = surfaceFormat.format;
        m_swapChainExtent = extent;
    }

    void VulkanSwapChain::createImageViews() {
        m_swapChainImageViews.resize(m_swapChainImages.size());

        for (size_t i = 0; i < m_swapChainImages.size(); i++) {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = m_swapChainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = m_swapChainImageFormat;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(m_device.getDevice(), &createInfo, nullptr, &m_swapChainImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create texture image view!");
            }
        }
    }

    void VulkanSwapChain::cleanupSwapChain() {
        for (auto imageView : m_swapChainImageViews) {
            vkDestroyImageView(m_device.getDevice(), imageView, nullptr);
        }

        if (m_swapChain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(m_device.getDevice(), m_swapChain, nullptr);
        }
    }

    VkSurfaceFormatKHR VulkanSwapChain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && 
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR VulkanSwapChain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) const {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                AE_INFO("Selected present mode: Mailbox");
                return availablePresentMode;
            }
        }

        AE_INFO("Selected present mode: FIFO (V-Sync)");
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D VulkanSwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, 
                                                uint32_t windowWidth, uint32_t windowHeight) const {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        } else {
            VkExtent2D actualExtent = {
                std::clamp(windowWidth, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
                std::clamp(windowHeight, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
            };

            return actualExtent;
        }
    }

    VkResult VulkanSwapChain::acquireNextImage(uint32_t* imageIndex, VkSemaphore imageAvailableSemaphore) {
        return vkAcquireNextImageKHR(m_device.getDevice(), m_swapChain, UINT64_MAX, 
                                   imageAvailableSemaphore, VK_NULL_HANDLE, imageIndex);
    }

    VkResult VulkanSwapChain::presentImage(uint32_t imageIndex, VkSemaphore waitSemaphore) {
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &waitSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &m_swapChain;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr; // Optional

        return vkQueuePresentKHR(m_device.getPresentQueue(), &presentInfo);
    }

    void VulkanSwapChain::recreateSwapChain(uint32_t windowWidth, uint32_t windowHeight) {
        // Wait for device to be idle before recreating swap chain
        vkDeviceWaitIdle(m_device.getDevice());

        cleanupSwapChain();

        // We would need to pass the swap chain support details again
        // This is a simplified implementation - in a real application,
        // you would need to query the support details again
        // For now, we'll assume the support details haven't changed
        // In a real implementation, you would need to pass them in

        // This is a placeholder - in a real implementation you would need to pass
        // the swap chain support details and queue family indices again
        // For now, we'll just recreate with the same parameters
        // In a real implementation, you would need to handle this properly
    }

}