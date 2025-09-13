#include "VulkanInstance.h"
#include "VulkanValidation.h"
#include "Core/Logger.h"
#include <SDL3/SDL.h>
#include <stdexcept>
#include <set>

namespace AstralEngine::VulkanR {

    VulkanInstance::VulkanInstance(const InstanceCreateInfo& createInfo) {
        createInstance(createInfo);
        setupDebugMessenger(createInfo.enableValidationLayers);
        AE_INFO("Vulkan instance created successfully");
    }

    VulkanInstance::~VulkanInstance() {
        if (m_debugMessenger != VK_NULL_HANDLE) {
            auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
                m_instance, "vkDestroyDebugUtilsMessengerEXT");
            if (func != nullptr) {
                func(m_instance, m_debugMessenger, nullptr);
            }
        }

        if (m_instance != VK_NULL_HANDLE) {
            vkDestroyInstance(m_instance, nullptr);
            AE_INFO("Vulkan instance destroyed");
        }
    }

    void VulkanInstance::createInstance(const InstanceCreateInfo& createInfo) {
        if (createInfo.enableValidationLayers && !checkValidationLayerSupport(createInfo.validationLayers)) {
            throw std::runtime_error("Validation layers requested, but not available!");
        }

        VkApplicationInfo appInfo = Initializers::ApplicationInfo(
            createInfo.appInfo.appName.c_str(),
            createInfo.appInfo.appVersion,
            createInfo.appInfo.engineName.c_str(),
            createInfo.appInfo.engineVersion,
            createInfo.appInfo.apiVersion
        );

        auto extensions = getRequiredExtensions(createInfo.enableValidationLayers);
        
        // Add user-specified extensions
        extensions.insert(extensions.end(), createInfo.extensions.begin(), createInfo.extensions.end());

        VkInstanceCreateInfo instanceCreateInfo = Initializers::InstanceCreateInfo(
            appInfo,
            extensions,
            createInfo.enableValidationLayers ? createInfo.validationLayers : std::vector<const char*>()
        );

        if (vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Vulkan instance!");
        }
    }

    void VulkanInstance::setupDebugMessenger(bool enableValidationLayers) {
        if (!enableValidationLayers) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = 
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = 
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = VulkanValidation::debugCallback;
        createInfo.pUserData = nullptr;

        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            m_instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            if (func(m_instance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS) {
                throw std::runtime_error("Failed to set up debug messenger!");
            }
        } else {
            throw std::runtime_error("Debug utils messenger extension not present!");
        }
    }

    bool VulkanInstance::checkValidationLayerSupport(const std::vector<const char*>& validationLayers) const {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : validationLayers) {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) {
                return false;
            }
        }

        return true;
    }

    std::vector<const char*> VulkanInstance::getRequiredExtensions(bool enableValidationLayers) const {
        unsigned int count;
        const char** extensions_c = SDL_Vulkan_GetInstanceExtensions(&count);
        std::vector<const char*> extensions(extensions_c, extensions_c + count);

        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    // Helper functions implementation
    namespace Initializers {
        VkApplicationInfo ApplicationInfo(
            const char* pApplicationName,
            uint32_t applicationVersion,
            const char* pEngineName,
            uint32_t engineVersion,
            uint32_t apiVersion
        ) {
            VkApplicationInfo appInfo{};
            appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            appInfo.pApplicationName = pApplicationName;
            appInfo.applicationVersion = applicationVersion;
            appInfo.pEngineName = pEngineName;
            appInfo.engineVersion = engineVersion;
            appInfo.apiVersion = apiVersion;
            return appInfo;
        }

        VkInstanceCreateInfo InstanceCreateInfo(
            const VkApplicationInfo& appInfo,
            const std::vector<const char*>& extensions,
            const std::vector<const char*>& layers
        ) {
            VkInstanceCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            createInfo.pApplicationInfo = &appInfo;
            createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
            createInfo.ppEnabledExtensionNames = extensions.data();
            createInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
            createInfo.ppEnabledLayerNames = layers.data();
            return createInfo;
        }
    }
}