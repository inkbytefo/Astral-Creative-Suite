#pragma once

#include <vulkan/vulkan.h>
#include <iostream>

namespace AstralEngine::VulkanR {

    class VulkanValidation {
    public:
        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData
        ) {
            if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
                // Log the message
                std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;
            }

            return VK_FALSE;
        }
    };

}