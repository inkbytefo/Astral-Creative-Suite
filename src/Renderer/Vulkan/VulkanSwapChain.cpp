#include "VulkanSwapChain.h"
#include "Platform/Window.h"
#include "Core/Logger.h"
#include <SDL3/SDL.h>

#include <stdexcept>
#include <algorithm>
#include <limits>

namespace AstralEngine::Vulkan {

    VulkanSwapChain::VulkanSwapChain(Vulkan::VulkanDevice& device, Window& window)
        : m_device(device), m_window(window) {
        createSwapChain();
        createImageViews();
        // Removed createRenderPass() as it's no longer needed with Dynamic Rendering
        // Removed createFramebuffers() as it's no longer needed with Dynamic Rendering
        m_maxFramesInFlight = m_swapChainImages.size(); // Dinamik frame sayısı ayarla
        createSyncObjects();
    }

    VulkanSwapChain::~VulkanSwapChain() {
        int framesInFlight = m_maxFramesInFlight;
        for (int i = 0; i < framesInFlight; i++) {
            if (i < m_imageAvailableSemaphores.size()) {
                vkDestroySemaphore(m_device.getDevice(), m_imageAvailableSemaphores[i], nullptr);
            }
            if (i < m_renderFinishedSemaphores.size()) {
                vkDestroySemaphore(m_device.getDevice(), m_renderFinishedSemaphores[i], nullptr);
            }
            if (i < m_inFlightFences.size()) {
                vkDestroyFence(m_device.getDevice(), m_inFlightFences[i], nullptr);
            }
        }

        // Removed framebuffer destruction as it's no longer needed with Dynamic Rendering
        // Removed render pass destruction as it's no longer needed with Dynamic Rendering

        for (auto imageView : m_swapChainImageViews) {
            vkDestroyImageView(m_device.getDevice(), imageView, nullptr);
        }

        if (m_swapChain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(m_device.getDevice(), m_swapChain, nullptr);
        }
    }

    void VulkanSwapChain::createSwapChain() {
        // Swap chain desteğini sorgula
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_device.getPhysicalDevice(), m_device.getSurface(), &formatCount, nullptr);
        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_device.getPhysicalDevice(), m_device.getSurface(), &formatCount, formats.data());

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_device.getPhysicalDevice(), m_device.getSurface(), &presentModeCount, nullptr);
        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_device.getPhysicalDevice(), m_device.getSurface(), &presentModeCount, presentModes.data());

        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_device.getPhysicalDevice(), m_device.getSurface(), &capabilities);

        // En iyi formatı, sunum modunu ve boyutu seç
        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(presentModes);
        VkExtent2D extent = chooseSwapExtent(capabilities);

        uint32_t imageCount = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
            imageCount = capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = m_device.getSurface();
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices = m_device.getQueueFamilyIndices();
        uint32_t queueFamilyIndices[] = {indices.graphicsFamily, indices.presentFamily};

        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        createInfo.preTransform = capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(m_device.getDevice(), &createInfo, nullptr, &m_swapChain) != VK_SUCCESS) {
            throw std::runtime_error("Swap chain oluşturulamadı!");
        }

        vkGetSwapchainImagesKHR(m_device.getDevice(), m_swapChain, &imageCount, nullptr);
        m_swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(m_device.getDevice(), m_swapChain, &imageCount, m_swapChainImages.data());

        m_swapChainImageFormat = surfaceFormat.format;
        m_swapChainExtent = extent;
        AE_INFO("Vulkan swap chain başarıyla oluşturuldu.");
    }

    void VulkanSwapChain::createImageViews() {
        m_swapChainImageViews.resize(m_swapChainImages.size());
        for (size_t i = 0; i < m_swapChainImages.size(); i++) {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = m_swapChainImages[i];
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = m_swapChainImageFormat;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(m_device.getDevice(), &viewInfo, nullptr, &m_swapChainImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("Swap chain image view oluşturulamadı!");
            }
        }
        AE_INFO("Swap chain image view'lar başarıyla oluşturuldu.");
    }

    // Removed createRenderPass() as it's no longer needed with Dynamic Rendering

    // Removed createFramebuffers() as it's no longer needed with Dynamic Rendering

    void VulkanSwapChain::createSyncObjects() {
        // frame başına senkronizasyon nesneleri
        m_imageAvailableSemaphores.resize(m_maxFramesInFlight);
        m_renderFinishedSemaphores.resize(m_maxFramesInFlight);
        m_inFlightFences.resize(m_maxFramesInFlight);

        // swapchain image'ları için inFlight map
        m_imagesInFlight.resize(m_swapChainImages.size(), VK_NULL_HANDLE);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // başlangıçta sinyalli, ilk frame için beklemeyi atlatmak için

        for (int i = 0; i < m_maxFramesInFlight; i++) {
            if (vkCreateSemaphore(m_device.getDevice(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(m_device.getDevice(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(m_device.getDevice(), &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS) {
                throw std::runtime_error("Senkronizasyon nesneleri oluşturulamadı!");
            }
        }
        AE_INFO("Senkronizasyon nesneleri (semaphores/fences) başarıyla oluşturuldu.");
    }

    VkResult VulkanSwapChain::AcquireNextImage(uint32_t* imageIndex) {
        // Önce frame'in fence'ini bekle (previous submit tamamlanana kadar aynı frame id kullanmıyoruz)
        vkWaitForFences(m_device.getDevice(), 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);
        vkResetFences(m_device.getDevice(), 1, &m_inFlightFences[m_currentFrame]);

        // Acquire: frame'e ait imageAvailable semaphore kullan
        VkResult result = vkAcquireNextImageKHR(
            m_device.getDevice(),
            m_swapChain,
            UINT64_MAX,
            m_imageAvailableSemaphores[m_currentFrame],
            VK_NULL_HANDLE,
            imageIndex);

        return result;
    }

    VkResult VulkanSwapChain::SubmitCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex) {
        uint32_t currentImage = *imageIndex;
        int currentFrame = m_currentFrame;

        // Eğer bu görüntü GPU'da hâlâ inFlight ise onu tutan fence'i bekle (image reuse kontrolü)
        if (m_imagesInFlight[currentImage] != VK_NULL_HANDLE) {
            vkWaitForFences(m_device.getDevice(), 1, &m_imagesInFlight[currentImage], VK_TRUE, UINT64_MAX);
        }

        // Bu image artık bu frame'in fence'i ile ilişkili olacak
        m_imagesInFlight[currentImage] = m_inFlightFences[currentFrame];

        // Submit hazırlığı: frame-semaphores kullan
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[currentFrame] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = buffers;

        VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[currentFrame] };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(m_device.getGraphicsQueue(), 1, &submitInfo, m_inFlightFences[currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("Komut tamponu gönderilemedi!");
        }

        // Present: renderFinished semaphore'ını beklet
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = { m_swapChain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = imageIndex;

        VkResult result = vkQueuePresentKHR(m_device.getPresentQueue(), &presentInfo);

        // frame index ilerlet
        m_currentFrame = (m_currentFrame + 1) % m_maxFramesInFlight;

        return result;
    }

    // --- Helper Fonksiyonlar ---

    VkSurfaceFormatKHR VulkanSwapChain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }
        return availableFormats[0];
    }

    VkPresentModeKHR VulkanSwapChain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                AE_INFO("Sunum Modu: Mailbox");
                return availablePresentMode;
            }
        }
        AE_INFO("Sunum Modu: V-Sync (Fifo)");
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D VulkanSwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        } else {
            int width, height;
            SDL_GetWindowSizeInPixels(m_window.getNativeWindow(), &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }
}