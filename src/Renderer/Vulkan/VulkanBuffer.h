#pragma once

#include "Renderer/Vulkan/VulkanDevice.h"
#include <vk_mem_alloc.h>
#include <string>

namespace AstralEngine::Vulkan {
	class VulkanBuffer {
	public:
		VulkanBuffer(VulkanDevice& device, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage vmaUsage);
		VulkanBuffer(VulkanDevice& device, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage vmaUsage, const std::string& name);
		~VulkanBuffer();

		VkBuffer getBuffer() const { return m_buffer; }
		VmaAllocation getAllocation() const { return m_allocation; }
		VkDeviceSize getSize() const { return m_size; }
		
		// Memory mapping methods
		void* map();
		void unmap();
		void copyTo(const void* data, VkDeviceSize size, VkDeviceSize offset = 0);
		void copyFrom(void* data, VkDeviceSize size, VkDeviceSize offset = 0) const;

	private:
		VulkanDevice& m_device;
		VkBuffer m_buffer;
		VmaAllocation m_allocation;
		VkDeviceSize m_size;
		void* m_mappedData = nullptr;
		std::string m_debugName = "VulkanBuffer";
	};
}