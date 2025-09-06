#pragma once

// 1. Ana Vulkan başlığını HER ZAMAN ilk olarak dahil et.
#include <vulkan/vulkan.h>

// 2. VMA'yı Vulkan'dan SONRA dahil et.
#include "vk_mem_alloc.h"

// Projenin diğer bileşenleri
#include "Platform/Window.h"
#include <memory>
#include <glm/glm.hpp>

namespace Astral::Vulkan {
    // İleride kullanılacak sınıflar için ön bildirimler
    class VulkanDevice;
    class VulkanSwapChain;

    struct UniformBufferObject {
        alignas(16) glm::mat4 model;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };

    // Tüm Vulkan altyapısını bir arada tutan yapı
    struct VulkanContext {
        VkInstance instance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VmaAllocator allocator = VK_NULL_HANDLE;
        
        std::unique_ptr<VulkanDevice> device;
        std::unique_ptr<VulkanSwapChain> swapChain;
    };
}
