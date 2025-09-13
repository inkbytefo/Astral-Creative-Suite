#include "Texture.h"
#include "Core/Logger.h"
#include "Renderer/VulkanR/VulkanBuffer.h"
#include "Renderer/VulkanR/VulkanDevice.h"
#include "Renderer/VulkanR/VulkanUtils.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <cctype>

namespace AstralEngine {

	// Removed - now using VulkanUtils::transitionImageLayout

	Texture::Texture(Vulkan::VulkanDevice& device, const std::string& filepath)
		: m_device(device) {
		// Auto-detect format and slot from filename
		m_slot = detectSlotFromPath(filepath);
		TextureFormat preferredFormat = getPreferredFormat(m_slot);
		m_format = getVulkanFormat(preferredFormat);
		
		loadFromFile(filepath, m_format);
		createSampler(m_mipLevels, m_slot);
	}

	Texture::Texture(Vulkan::VulkanDevice& device, const std::string& filepath, TextureFormat format, TextureSlot slot)
		: m_device(device), m_slot(slot) {
		m_format = getVulkanFormat(format);
		loadFromFile(filepath, m_format);
		createSampler(m_mipLevels, m_slot);
	}

	Texture::Texture(Vulkan::VulkanDevice& device, uint32_t width, uint32_t height, VkFormat format, const void* data)
		: m_device(device), m_width(width), m_height(height), m_format(format), m_slot(TextureSlot::BaseColor) {
		createFromData(width, height, format, data);
		createSampler(m_mipLevels, m_slot);
	}

	Texture::~Texture() {
		vkDestroySampler(m_device.getDevice(), m_sampler, nullptr);
		vkDestroyImageView(m_device.getDevice(), m_imageView, nullptr);
		vmaDestroyImage(m_device.getAllocator(), m_image, m_imageAllocation);
	}

	void Texture::createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VmaMemoryUsage vmaUsage) {
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = mipLevels;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = vmaUsage;

		vmaCreateImage(m_device.getAllocator(), &imageInfo, &allocInfo, &m_image, &m_imageAllocation, nullptr);
	}

	void Texture::createImageView(VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) {
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = m_image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.subresourceRange.aspectMask = aspectFlags;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = mipLevels;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(m_device.getDevice(), &viewInfo, nullptr, &m_imageView) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture image view!");
		}
	}

	void Texture::createSampler(uint32_t mipLevels, TextureSlot slot) {
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		
		// Configure addressing mode based on texture slot
		if (slot == TextureSlot::Normal) {
			// Normal maps typically don't need repeat
			samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		} else {
			samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		}
		
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = 16;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = static_cast<float>(mipLevels);

		if (vkCreateSampler(m_device.getDevice(), &samplerInfo, nullptr, &m_sampler) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture sampler!");
		}
	}

	void Texture::generateMipmaps(VkImage image, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {
		VkCommandBuffer commandBuffer = m_device.beginSingleTimeCommands();

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = image;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.levelCount = 1;

		int32_t mipWidth = texWidth;
		int32_t mipHeight = texHeight;

		for (uint32_t i = 1; i < mipLevels; i++) {
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			VkImageBlit blit{};
			blit.srcOffsets[0] = {0, 0, 0};
			blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = i - 1;
			blit.srcSubresource.baseArrayLayer = 0;
			blit.srcSubresource.layerCount = 1;
			blit.dstOffsets[0] = {0, 0, 0};
			blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = 0;
			blit.dstSubresource.layerCount = 1;

			vkCmdBlitImage(commandBuffer,
				image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &blit,
				VK_FILTER_LINEAR);

			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			if (mipWidth > 1) mipWidth /= 2;
			if (mipHeight > 1) mipHeight /= 2;
		}

		barrier.subresourceRange.baseMipLevel = mipLevels - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		m_device.endSingleTimeCommands(commandBuffer);
	}

	void Texture::loadFromFile(const std::string& filepath, VkFormat format) {
		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load(filepath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		VkDeviceSize imageSize = texWidth * texHeight * 4;
		m_width = texWidth;
		m_height = texHeight;
		m_mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

		if (!pixels) {
			AE_ERROR("Failed to load texture image: {}", filepath);
			return;
		}

		Vulkan::VulkanBuffer stagingBuffer(m_device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

		void* data;
		vmaMapMemory(m_device.getAllocator(), stagingBuffer.getAllocation(), &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vmaUnmapMemory(m_device.getAllocator(), stagingBuffer.getAllocation());

		stbi_image_free(pixels);

		createImage(texWidth, texHeight, m_mipLevels, format, VK_IMAGE_TILING_OPTIMAL, 
					VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
					VMA_MEMORY_USAGE_GPU_ONLY);

		Vulkan::VulkanUtils::transitionImageLayout(m_device, m_image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_mipLevels);
		m_device.copyBufferToImage(stagingBuffer.getBuffer(), m_image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
		generateMipmaps(m_image, texWidth, texHeight, m_mipLevels);

		createImageView(format, VK_IMAGE_ASPECT_COLOR_BIT, m_mipLevels);
	}

	void Texture::createFromData(uint32_t width, uint32_t height, VkFormat format, const void* data) {
		m_width = width;
		m_height = height;
		m_mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
		
		VkDeviceSize imageSize = width * height * 4; // Assuming 4 bytes per pixel
		
		// Create image
		createImage(width, height, m_mipLevels, format, VK_IMAGE_TILING_OPTIMAL,
					VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
					VMA_MEMORY_USAGE_GPU_ONLY);
		
		if (data) {
			// Upload data if provided
			Vulkan::VulkanBuffer stagingBuffer(m_device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
			
			void* mapped;
			vmaMapMemory(m_device.getAllocator(), stagingBuffer.getAllocation(), &mapped);
			memcpy(mapped, data, static_cast<size_t>(imageSize));
			vmaUnmapMemory(m_device.getAllocator(), stagingBuffer.getAllocation());
			
			Vulkan::VulkanUtils::transitionImageLayout(m_device, m_image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_mipLevels);
			m_device.copyBufferToImage(stagingBuffer.getBuffer(), m_image, width, height);
			generateMipmaps(m_image, width, height, m_mipLevels);
		} else {
			// Just transition to shader read layout
			Vulkan::VulkanUtils::transitionImageLayout(m_device, m_image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_mipLevels);
		}
		
		createImageView(format, VK_IMAGE_ASPECT_COLOR_BIT, m_mipLevels);
	}

	VkFormat Texture::getVulkanFormat(TextureFormat format) {
		switch (format) {
			case TextureFormat::SRGB:
				return VK_FORMAT_R8G8B8A8_SRGB;
			case TextureFormat::LINEAR:
				return VK_FORMAT_R8G8B8A8_UNORM;
			case TextureFormat::AUTO:
			default:
				return VK_FORMAT_R8G8B8A8_SRGB;
		}
	}

	TextureFormat Texture::detectFormatFromPath(const std::string& filepath) {
		// Convert to lowercase for case-insensitive comparison
		std::string lowercasePath = filepath;
		std::transform(lowercasePath.begin(), lowercasePath.end(), lowercasePath.begin(), ::tolower);
		
		// Check for format indicators in filename
		if (lowercasePath.find("normal") != std::string::npos ||
			lowercasePath.find("_n.") != std::string::npos ||
			lowercasePath.find("metallic") != std::string::npos ||
			lowercasePath.find("roughness") != std::string::npos ||
			lowercasePath.find("ao") != std::string::npos ||
			lowercasePath.find("occlusion") != std::string::npos) {
			return TextureFormat::LINEAR;
		}
		
		return TextureFormat::SRGB; // Default to sRGB for albedo, emission, etc.
	}

	TextureSlot Texture::detectSlotFromPath(const std::string& filepath) {
		// Convert to lowercase for case-insensitive comparison
		std::string lowercasePath = filepath;
		std::transform(lowercasePath.begin(), lowercasePath.end(), lowercasePath.begin(), ::tolower);
		
	if (lowercasePath.find("normal") != std::string::npos || lowercasePath.find("_n.") != std::string::npos) {
		return TextureSlot::Normal;
	}
		if (lowercasePath.find("metallic") != std::string::npos || lowercasePath.find("roughness") != std::string::npos || 
			lowercasePath.find("_mr.") != std::string::npos || lowercasePath.find("_orm.") != std::string::npos) {
		return TextureSlot::MetallicRoughness;
		}
		if (lowercasePath.find("emissive") != std::string::npos || lowercasePath.find("emission") != std::string::npos ||
			lowercasePath.find("_e.") != std::string::npos) {
		return TextureSlot::Emissive;
		}
		if (lowercasePath.find("ao") != std::string::npos || lowercasePath.find("occlusion") != std::string::npos) {
		return TextureSlot::Occlusion;
		}
		
	return TextureSlot::BaseColor; // Default to base color
	}
}
