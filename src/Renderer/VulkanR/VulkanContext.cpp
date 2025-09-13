#include "VulkanContext.h"
#include "Core/Logger.h"
#include <SDL3/SDL.h>
#include <stdexcept>

namespace AstralEngine::VulkanR {

    VulkanContext::VulkanContext(const ContextCreateInfo& createInfo) {
        // Create Vulkan instance
        m_instance = std::make_unique<VulkanInstance>(createInfo.instanceCreateInfo);
        
        // Create surface
        createSurface(createInfo.window);
        
        // Create physical device
        m_physicalDevice = std::make_unique<VulkanPhysicalDevice>(m_instance->getInstance(), m_surface);
        
        // Select best physical device
        VulkanPhysicalDevice::DeviceRequirements deviceRequirements;
        deviceRequirements.requiredExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
        deviceRequirements.requiredFeatures.samplerAnisotropy = VK_TRUE;
        
        VkPhysicalDevice physicalDevice = m_physicalDevice->getBestDevice(deviceRequirements);
        
        // Create logical device
        VulkanDevice::DeviceCreateInfo deviceCreateInfo;
        deviceCreateInfo.physicalDevice = physicalDevice;
        deviceCreateInfo.queueFamilyIndices = m_physicalDevice->getQueueFamilyIndices();
        deviceCreateInfo.requiredExtensions = deviceRequirements.requiredExtensions;
        deviceCreateInfo.requiredFeatures = deviceRequirements.requiredFeatures;
        deviceCreateInfo.enableValidationLayers = createInfo.instanceCreateInfo.enableValidationLayers;
        
        m_device = std::make_unique<VulkanDevice>(deviceCreateInfo);
        
        // Create swap chain
        VulkanSwapChain::SwapChainCreateInfo swapChainCreateInfo;
        swapChainCreateInfo.surface = m_surface;
        swapChainCreateInfo.windowWidth = createInfo.windowWidth;
        swapChainCreateInfo.windowHeight = createInfo.windowHeight;
        swapChainCreateInfo.swapChainSupport = m_physicalDevice->getSwapChainSupport();
        swapChainCreateInfo.queueFamilyIndices = m_physicalDevice->getQueueFamilyIndices();
        
        m_swapChain = std::make_unique<VulkanSwapChain>(*m_device, swapChainCreateInfo);
        
        AE_INFO("Vulkan context created successfully");
    }

    VulkanContext::~VulkanContext() {
        // Clean up in reverse order
        m_swapChain.reset();
        m_device.reset();
        m_physicalDevice.reset();
        
        if (m_surface != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(m_instance->getInstance(), m_surface, nullptr);
        }
        
        m_instance.reset();
        
        AE_INFO("Vulkan context destroyed");
    }

    void VulkanContext::createSurface(SDL_Window* window) {
        if (SDL_Vulkan_CreateSurface(window, m_instance->getInstance(), nullptr, &m_surface) != SDL_TRUE) {
            throw std::runtime_error("Failed to create Vulkan surface!");
        }
    }

}