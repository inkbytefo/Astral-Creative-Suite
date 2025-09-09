#pragma once

// 1. Ana Vulkan başlığını HER ZAMAN ilk olarak dahil et.
#include <vulkan/vulkan.h>

// 2. VMA'yı Vulkan'dan SONRA dahil et.
// Forward declaration for VmaAllocator
struct VmaAllocator_T;
typedef struct VmaAllocator_T* VmaAllocator;

// Projenin diğer bileşenleri
#include "Platform/Window.h"
#include <memory>
#include <glm/glm.hpp>

namespace AstralEngine::Vulkan {
    // İleride kullanılacak sınıflar için ön bildirimler
    class VulkanDevice;
    class VulkanSwapChain;

    struct UniformBufferObject {
        // Model matrisini kaldırın çünkü artık push constant olarak gönderilecek
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };

    // Tüm Vulkan altyapısını bir arada tutan yapı
    struct VulkanContext {
        VkInstance instance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        
        std::unique_ptr<VulkanDevice> device;
        std::unique_ptr<VulkanSwapChain> swapChain;
    };
}