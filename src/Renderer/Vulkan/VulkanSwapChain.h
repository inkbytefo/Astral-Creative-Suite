#pragma once

#include "VulkanDevice.h"
#include "Platform/Window.h"
#include <vector>

namespace Astral {
    class Window; // Forward declaration
}

namespace AstralEngine::Vulkan {
    class VulkanSwapChain {
    public:
        VulkanSwapChain(Vulkan::VulkanDevice& device, Window& window);
        ~VulkanSwapChain();

        VulkanSwapChain(const VulkanSwapChain&) = delete;
        VulkanSwapChain& operator=(const VulkanSwapChain&) = delete;

        // Removed getRenderPass() as it's no longer needed with Dynamic Rendering
        // Removed getFrameBuffer() as it's no longer needed with Dynamic Rendering
        VkExtent2D getSwapChainExtent() const { return m_swapChainExtent; }
        size_t getImageCount() const { return m_swapChainImages.size(); }
        VkFormat getSwapChainImageFormat() const { return m_swapChainImageFormat; }
        const std::vector<VkImageView>& getImageViews() const { return m_swapChainImageViews; }
        const std::vector<VkImage>& getImages() const { return m_swapChainImages; }

        VkResult AcquireNextImage(uint32_t* imageIndex);
        VkResult SubmitCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex);

    private:
        void createSwapChain();
        void createImageViews();
        // Removed createRenderPass() as it's no longer needed with Dynamic Rendering
        // Removed createFramebuffers() as it's no longer needed with Dynamic Rendering
        void createSyncObjects();

        // Helper fonksiyonlar
        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

        Vulkan::VulkanDevice& m_device;
        Window& m_window;

        VkSwapchainKHR m_swapChain;
        std::vector<VkImage> m_swapChainImages;
        VkFormat m_swapChainImageFormat;
        VkExtent2D m_swapChainExtent;
        std::vector<VkImageView> m_swapChainImageViews;
        // Removed m_swapChainFramebuffers as it's no longer needed with Dynamic Rendering
        // Removed m_renderPass as it's no longer needed with Dynamic Rendering

        // Senkronizasyon
        std::vector<VkSemaphore> m_imageAvailableSemaphores;
        std::vector<VkSemaphore> m_renderFinishedSemaphores;
        std::vector<VkFence> m_inFlightFences;
        std::vector<VkFence> m_imagesInFlight; // her swapchain görüntüsü için hangi fence rezerve edildi
        uint32_t m_currentFrame = 0;
        int m_maxFramesInFlight = 0; // Dinamik olarak swapchain image sayısına göre ayarlanır
    };
}