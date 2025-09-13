#pragma once

// Forward declaration
namespace AstralEngine {
    namespace Vulkan {
        class VulkanDevice;
    }
}
#include <vulkan/vulkan.h>
#pragma once

#include "Renderer/VulkanR/Vulkan.h"
#include <string>
#include "Renderer/UnifiedMaterialConstants.h"

namespace AstralEngine {
	class Texture {
	public:
		// Constructor with automatic format detection
		Texture(Vulkan::VulkanDevice& device, const std::string& filepath);
		// Constructor with explicit format and slot specification
        Texture(Vulkan::VulkanDevice& device, const std::string& filepath, TextureFormat format, TextureSlot slot = TextureSlot::BaseColor);
		// Constructor for manual texture creation
		Texture(Vulkan::VulkanDevice& device, uint32_t width, uint32_t height, VkFormat format, const void* data = nullptr);
		~Texture();

		VkImageView getImageView() const { return m_imageView; }
		VkSampler getSampler() const { return m_sampler; }
		VkFormat getFormat() const { return m_format; }
		TextureSlot getSlot() const { return m_slot; }
		uint32_t getWidth() const { return m_width; }
		uint32_t getHeight() const { return m_height; }
		uint32_t getMipLevels() const { return m_mipLevels; }
		
		// Static utility methods
		static VkFormat getVulkanFormat(TextureFormat format);
		static TextureFormat detectFormatFromPath(const std::string& filepath);
		static TextureSlot detectSlotFromPath(const std::string& filepath);

	private:
		void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VmaMemoryUsage vmaUsage);
		void createImageView(VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
        void createSampler(uint32_t mipLevels, TextureSlot slot = TextureSlot::BaseColor);
		void generateMipmaps(VkImage image, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
		void loadFromFile(const std::string& filepath, VkFormat format);
		void createFromData(uint32_t width, uint32_t height, VkFormat format, const void* data);

		Vulkan::VulkanDevice& m_device;
		VkImage m_image;
		VmaAllocation m_imageAllocation;
		VkImageView m_imageView;
		VkSampler m_sampler;
		uint32_t m_mipLevels;
		uint32_t m_width;
		uint32_t m_height;
		VkFormat m_format;
		TextureSlot m_slot;
	};
}