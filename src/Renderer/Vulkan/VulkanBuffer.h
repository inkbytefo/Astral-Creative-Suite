#pragma once
#include "VulkanContext.h"
#include "vk_mem_alloc.h" // VMA'yı dahil et

namespace Astral::Vulkan {
    struct VulkanBuffer {
        VkBuffer buffer = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
    };
}