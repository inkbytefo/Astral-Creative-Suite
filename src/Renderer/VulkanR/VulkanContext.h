#pragma once

#include "VulkanInstance.h"
#include "VulkanPhysicalDevice.h"
#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include <memory>

struct SDL_Window;

namespace AstralEngine::VulkanR {

    class VulkanContext {
    public:
        struct ContextCreateInfo {
            VulkanInstance::InstanceCreateInfo instanceCreateInfo;
            uint32_t windowWidth;
            uint32_t windowHeight;
            SDL_Window* window;
        };

        VulkanContext(const ContextCreateInfo& createInfo);
        ~VulkanContext();

        // Non-copyable
        VulkanContext(const VulkanContext&) = delete;
        VulkanContext& operator=(const VulkanContext&) = delete;

        VulkanInstance& getInstance() { return *m_instance; }
        VulkanPhysicalDevice& getPhysicalDevice() { return *m_physicalDevice; }
        VulkanDevice& getDevice() { return *m_device; }
        VulkanSwapChain& getSwapChain() { return *m_swapChain; }

        VkSurfaceKHR getSurface() const { return m_surface; }

    private:
        void createSurface(SDL_Window* window);

        std::unique_ptr<VulkanInstance> m_instance;
        VkSurfaceKHR m_surface;
        std::unique_ptr<VulkanPhysicalDevice> m_physicalDevice;
        std::unique_ptr<VulkanDevice> m_device;
        std::unique_ptr<VulkanSwapChain> m_swapChain;
    };

}