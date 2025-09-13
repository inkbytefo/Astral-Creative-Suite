#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <string>

namespace AstralEngine::VulkanR {

    class VulkanInstance {
    public:
        struct ApplicationInfo {
            std::string appName = "Astral Creative Suite";
            uint32_t appVersion = VK_MAKE_VERSION(1, 0, 0);
            std::string engineName = "Astral Engine";
            uint32_t engineVersion = VK_MAKE_VERSION(1, 0, 0);
            uint32_t apiVersion = VK_API_VERSION_1_3;
        };

        struct InstanceCreateInfo {
            ApplicationInfo appInfo;
            std::vector<const char*> extensions;
            std::vector<const char*> validationLayers;
            bool enableValidationLayers = true;
        };

        VulkanInstance(const InstanceCreateInfo& createInfo);
        ~VulkanInstance();

        // Non-copyable
        VulkanInstance(const VulkanInstance&) = delete;
        VulkanInstance& operator=(const VulkanInstance&) = delete;

        VkInstance getInstance() const { return m_instance; }
        VkDebugUtilsMessengerEXT getDebugMessenger() const { return m_debugMessenger; }

    private:
        void createInstance(const InstanceCreateInfo& createInfo);
        void setupDebugMessenger(bool enableValidationLayers);
        bool checkValidationLayerSupport(const std::vector<const char*>& validationLayers) const;
        std::vector<const char*> getRequiredExtensions(bool enableValidationLayers) const;

        VkInstance m_instance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
    };

    // Helper functions for Vulkan initializers
    namespace Initializers {
        VkApplicationInfo ApplicationInfo(
            const char* pApplicationName,
            uint32_t applicationVersion,
            const char* pEngineName,
            uint32_t engineVersion,
            uint32_t apiVersion
        );

        VkInstanceCreateInfo InstanceCreateInfo(
            const VkApplicationInfo& appInfo,
            const std::vector<const char*>& extensions,
            const std::vector<const char*>& layers
        );
    }
}