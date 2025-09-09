#pragma once

#include "Renderer/Vulkan/VulkanDevice.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include <unordered_map>
#include <string>

namespace AstralEngine::Vulkan {

	// Pool statistics for monitoring and debugging
	struct DescriptorPoolStats {
		uint32_t totalSets = 0;
		uint32_t allocatedSets = 0;
		uint32_t freedSets = 0;
		uint32_t failedAllocations = 0;
		uint32_t poolsCreated = 0;
		std::unordered_map<VkDescriptorType, uint32_t> typeUsage;
	};

	// Scene descriptor set configuration (set=0)
	struct SceneDescriptorConfig {
		VkDescriptorSetLayout layout = VK_NULL_HANDLE;
		std::vector<VkDescriptorSet> descriptorSets; // Per swapchain image
		bool needsRebuild = true;
	};

	class DescriptorSetManager {
	public:
		DescriptorSetManager(VulkanDevice& device);
		~DescriptorSetManager();

		// Layout management
		VkDescriptorSetLayout getDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings);
		VkDescriptorSetLayout getDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings, const std::string& name);

		// Descriptor set allocation
		VkDescriptorSet allocateDescriptorSet(VkDescriptorSetLayout layout);
		VkDescriptorSet allocateDescriptorSet(VkDescriptorSetLayout layout, const std::string& debugName);
		void updateDescriptorSet(VkDescriptorSet descriptorSet, const std::vector<VkWriteDescriptorSet>& writes);
		
		// Descriptor set freeing
		void freeDescriptorSet(VkDescriptorSet descriptorSet);
		void freeDescriptorSets(const std::vector<VkDescriptorSet>& descriptorSets);
		void processDeferredDeletions(); // Call at the beginning of each frame

		// Scene descriptor set management (set=0)
		void initializeSceneDescriptors(const std::vector<VkDescriptorSetLayoutBinding>& bindings, size_t swapchainImageCount);
		VkDescriptorSet getSceneDescriptorSet(size_t imageIndex) const;
		VkDescriptorSetLayout getSceneDescriptorLayout() const { return m_sceneConfig.layout; }
		void updateSceneDescriptorSets(size_t imageIndex, const std::vector<VkWriteDescriptorSet>& writes);
		bool sceneDescriptorsReady() const { return m_sceneConfig.layout != VK_NULL_HANDLE && !m_sceneConfig.needsRebuild; }

		// Statistics and monitoring
		const DescriptorPoolStats& getStats() const { return m_stats; }
		void resetStats() { m_stats = {}; }
		void logStats() const;

	private:
		void createDescriptorPool();
		void createDescriptorPool(uint32_t maxSets, const std::vector<VkDescriptorPoolSize>& poolSizes);
		size_t hashBindings(const std::vector<VkDescriptorSetLayoutBinding>& bindings) const;
		void updateStats(VkDescriptorType type, bool success);
		
		VulkanDevice& m_device;
		std::vector<VkDescriptorPool> m_descriptorPools;
		VkDescriptorPool m_currentPool;

		// Layout caching
		std::unordered_map<size_t, VkDescriptorSetLayout> m_layoutCache;
		std::unordered_map<size_t, std::string> m_layoutNames; // For debugging
		
		// Descriptor set tracking for freeing
		std::unordered_map<VkDescriptorSet, VkDescriptorPool> m_setToPool; // Track which pool each set belongs to
		std::vector<VkDescriptorSet> m_pendingDeletions; // Deferred deletion queue
		
		// Scene descriptor management
		SceneDescriptorConfig m_sceneConfig;
		
		// Statistics
		DescriptorPoolStats m_stats;
		
		// Pool configuration
		uint32_t m_maxSetsPerPool = 1000;
		uint32_t m_maxDescriptorsPerType = 1000;
	};
}