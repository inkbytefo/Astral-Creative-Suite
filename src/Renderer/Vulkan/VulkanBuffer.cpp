#include "VulkanBuffer.h"
#include "Core/Logger.h"
#include <cstring>

namespace AstralEngine::Vulkan {
	VulkanBuffer::VulkanBuffer(VulkanDevice& device, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage vmaUsage)
		: m_device(device), m_size(size), m_buffer(VK_NULL_HANDLE), m_allocation(VK_NULL_HANDLE), m_debugName("VulkanBuffer")
	{
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = vmaUsage;

		vmaCreateBuffer(m_device.getAllocator(), &bufferInfo, &allocInfo, &m_buffer, &m_allocation, nullptr);
	}
	
	VulkanBuffer::VulkanBuffer(VulkanDevice& device, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage vmaUsage, const std::string& name)
		: m_device(device), m_size(size), m_buffer(VK_NULL_HANDLE), m_allocation(VK_NULL_HANDLE), m_debugName(name)
	{
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = vmaUsage;

		vmaCreateBuffer(m_device.getAllocator(), &bufferInfo, &allocInfo, &m_buffer, &m_allocation, nullptr);
	}

	VulkanBuffer::~VulkanBuffer() {
		// Unmap if still mapped
		if (m_mappedData) {
			unmap();
		}
		
		// Only destroy if we have valid handles
		if (m_buffer != VK_NULL_HANDLE && m_allocation != VK_NULL_HANDLE && m_allocation != nullptr) {
			vmaDestroyBuffer(m_device.getAllocator(), m_buffer, m_allocation);
			m_buffer = VK_NULL_HANDLE;
			m_allocation = VK_NULL_HANDLE;
		}
	}
	
	void* VulkanBuffer::map() {
		if (m_mappedData) {
			AE_WARN("Buffer {} is already mapped", m_debugName);
			return m_mappedData;
		}
		
		VkResult result = vmaMapMemory(m_device.getAllocator(), m_allocation, &m_mappedData);
		if (result != VK_SUCCESS) {
			AE_ERROR("Failed to map buffer {}: {}", m_debugName, result);
			m_mappedData = nullptr;
		}
		
		return m_mappedData;
	}
	
	void VulkanBuffer::unmap() {
		if (!m_mappedData) {
			AE_WARN("Buffer {} is not mapped", m_debugName);
			return;
		}
		
		vmaUnmapMemory(m_device.getAllocator(), m_allocation);
		m_mappedData = nullptr;
	}
	
	void VulkanBuffer::copyTo(const void* data, VkDeviceSize size, VkDeviceSize offset) {
		if (!m_mappedData) {
			AE_ERROR("Buffer {} must be mapped before copying data", m_debugName);
			return;
		}
		
		if (offset + size > m_size) {
			AE_ERROR("Buffer {} copy would exceed buffer size: {} + {} > {}", 
				m_debugName, offset, size, m_size);
			return;
		}
		
		std::memcpy(static_cast<char*>(m_mappedData) + offset, data, size);
	}
	
	void VulkanBuffer::copyFrom(void* data, VkDeviceSize size, VkDeviceSize offset) const {
		if (!m_mappedData) {
			AE_ERROR("Buffer {} must be mapped before copying data", m_debugName);
			return;
		}
		
		if (offset + size > m_size) {
			AE_ERROR("Buffer {} copy would exceed buffer size: {} + {} > {}", 
				m_debugName, offset, size, m_size);
			return;
		}
		
		std::memcpy(data, static_cast<const char*>(m_mappedData) + offset, size);
	}
}