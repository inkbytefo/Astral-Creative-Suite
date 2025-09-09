#pragma once

#include "VulkanDevice.h"
#include <vulkan/vulkan.h>
#include <string>

namespace AstralEngine::Vulkan {

    /**
     * Utility functions for common Vulkan operations
     */
    class VulkanUtils {
    public:
        /**
         * Transitions an image from one layout to another using a pipeline barrier
         * @param device The Vulkan device to use for the operation
         * @param image The image to transition
         * @param format The format of the image
         * @param oldLayout The current layout of the image
         * @param newLayout The desired layout of the image
         * @param mipLevels Number of mip levels to transition (default: 1)
         */
        static void transitionImageLayout(
            VulkanDevice& device,
            VkImage image,
            VkFormat format,
            VkImageLayout oldLayout,
            VkImageLayout newLayout,
            uint32_t mipLevels = 1
        );

        /**
         * Creates a Vulkan exception with detailed error information
         * @param result The VkResult error code
         * @param operation The operation that failed
         * @param file The source file where the error occurred
         * @param line The line number where the error occurred
         * @return A formatted error message
         */
        static std::string formatVulkanError(
            VkResult result,
            const std::string& operation,
            const char* file = nullptr,
            int line = 0
        );

        /**
         * Gets a human-readable string for a VkResult error code
         * @param result The VkResult to convert
         * @return String representation of the error
         */
        static const char* vkResultToString(VkResult result);

        /**
         * Checks if a VkResult indicates success, throws VulkanException if not
         * @param result The VkResult to check
         * @param operation Description of the operation that was attempted
         * @param file Source file where the check occurred
         * @param line Line number where the check occurred
         */
        static void checkResult(
            VkResult result,
            const std::string& operation,
            const char* file = __FILE__,
            int line = __LINE__
        );

    private:
        VulkanUtils() = delete; // Static class, no instantiation
    };

    // Convenience macro for checking Vulkan results
    #define VK_CHECK(result, operation) \
        AstralEngine::Vulkan::VulkanUtils::checkResult(result, operation, __FILE__, __LINE__)

} // namespace AstralEngine::Vulkan
