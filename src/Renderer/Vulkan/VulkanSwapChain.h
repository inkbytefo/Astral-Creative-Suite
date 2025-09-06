#pragma once

#include "VulkanContext.h"
#include <vector>

namespace Astral {
    class Window; // Forward declaration
}

namespace Astral::Vulkan {
    class VulkanSwapChain {
    public:
        VulkanSwapChain(VulkanDevice& device, Window& window);
        ~VulkanSwapChain();

        VulkanSwapChain(const VulkanSwapChain&) = delete;
        VulkanSwapChain& operator=(const VulkanSwapChain&) = delete;

        VkRenderPass GetRenderPass() const { return m_renderPass; }
        VkFramebuffer GetFrameBuffer(int index) const { return m_swapChainFramebuffers[index]; }
        VkExtent2D GetSwapChainExtent() const { return m_swapChainExtent; }
        size_t GetImageCount() const { return m_swapChainImages.size(); }

        VkResult AcquireNextImage(uint32_t* imageIndex);
        VkResult SubmitCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex);

    private:
        void createSwapChain();
        void createImageViews();
        void createRenderPass();
        void createFramebuffers();
        void createSyncObjects();

        // Helper fonksiyonlar
        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

        VulkanDevice& m_device;
        Window& m_window;

        VkSwapchainKHR m_swapChain;
        std::vector<VkImage> m_swapChainImages;
        VkFormat m_swapChainImageFormat;
        VkExtent2D m_swapChainExtent;
        std::vector<VkImageView> m_swapChainImageViews;
        std::vector<VkFramebuffer> m_swapChainFramebuffers;

        VkRenderPass m_renderPass;

        // Senkronizasyon
        std::vector<VkSemaphore> m_imageAvailableSemaphores;
        std::vector<VkSemaphore> m_renderFinishedSemaphores;
        std::vector<VkFence> m_inFlightFences;
        uint32_t m_currentFrame = 0;
        const int MAX_FRAMES_IN_FLIGHT = 2;
    };
}
