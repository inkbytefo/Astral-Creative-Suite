// Bu dosya, VMA kütüphanesinin implementasyonunu içerir.
// Projede sadece bir kez derlenmelidir.

// Önce VulkanContext'i dahil ederek tüm Vulkan tanımlarının hazır olmasını sağlıyoruz.
#include "Vulkan/VulkanContext.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
