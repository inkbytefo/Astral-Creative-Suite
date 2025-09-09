#include "DescriptorSetManager.h"
#include "Core/Logger.h"
#include <functional>
#include <stdexcept>

namespace AstralEngine::Vulkan {

	size_t DescriptorSetManager::hashBindings(const std::vector<VkDescriptorSetLayoutBinding>& bindings) const {
		size_t seed = 0;
		for (const auto& binding : bindings) {
			seed ^= std::hash<uint32_t>()(binding.binding) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			seed ^= std::hash<uint32_t>()(binding.descriptorType) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			seed ^= std::hash<uint32_t>()(binding.descriptorCount) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			seed ^= std::hash<uint32_t>()(binding.stageFlags) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		}
		return seed;
	}

	DescriptorSetManager::DescriptorSetManager(VulkanDevice& device)
		: m_device(device), m_currentPool(VK_NULL_HANDLE), m_stats{} {
		// Create initial descriptor pool
		createDescriptorPool();
		AE_INFO("DescriptorSetManager initialized");
	}

	DescriptorSetManager::~DescriptorSetManager() {
		// Clean up scene descriptors
		if (m_sceneConfig.layout != VK_NULL_HANDLE) {
			vkDestroyDescriptorSetLayout(m_device.getDevice(), m_sceneConfig.layout, nullptr);
		}
		
		for (auto& pair : m_layoutCache) {
			vkDestroyDescriptorSetLayout(m_device.getDevice(), pair.second, nullptr);
		}
		
		// Destroy all descriptor pools
		for (auto& pool : m_descriptorPools) {
			vkDestroyDescriptorPool(m_device.getDevice(), pool, nullptr);
		}
		
	AE_INFO("DescriptorSetManager destroyed. Final stats: {} sets allocated, {} sets freed, {} failed allocations, {} pools created", 
		m_stats.allocatedSets, m_stats.freedSets, m_stats.failedAllocations, m_stats.poolsCreated);
	}

	void DescriptorSetManager::createDescriptorPool() {
		std::vector<VkDescriptorPoolSize> poolSizes = {
			{ VK_DESCRIPTOR_TYPE_SAMPLER, m_maxDescriptorsPerType },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_maxDescriptorsPerType },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, m_maxDescriptorsPerType },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, m_maxDescriptorsPerType },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, m_maxDescriptorsPerType },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, m_maxDescriptorsPerType },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_maxDescriptorsPerType },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_maxDescriptorsPerType },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, m_maxDescriptorsPerType },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, m_maxDescriptorsPerType },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, m_maxDescriptorsPerType }
		};
		createDescriptorPool(m_maxSetsPerPool, poolSizes);
	}

	void DescriptorSetManager::createDescriptorPool(uint32_t maxSets, const std::vector<VkDescriptorPoolSize>& poolSizes) {
		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		poolInfo.maxSets = maxSets;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();

		VkDescriptorPool pool;
		if (vkCreateDescriptorPool(m_device.getDevice(), &poolInfo, nullptr, &pool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor pool!");
		}
		
		m_descriptorPools.push_back(pool);
		m_currentPool = pool;
		m_stats.poolsCreated++;
		m_stats.totalSets += maxSets;
		AE_INFO("Created descriptor pool with {} max sets", maxSets);
	}

	VkDescriptorSetLayout DescriptorSetManager::getDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings) {
		return getDescriptorSetLayout(bindings, "unnamed_layout");
	}

	VkDescriptorSetLayout DescriptorSetManager::getDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings, const std::string& name) {
		size_t hash = hashBindings(bindings);
		if (m_layoutCache.find(hash) != m_layoutCache.end()) {
			return m_layoutCache[hash];
		}

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		VkDescriptorSetLayout layout;
		if (vkCreateDescriptorSetLayout(m_device.getDevice(), &layoutInfo, nullptr, &layout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor set layout!");
		}

		m_layoutCache[hash] = layout;
		m_layoutNames[hash] = name;
		AE_INFO("Created descriptor set layout: {}", name);
		return layout;
	}

	VkDescriptorSet DescriptorSetManager::allocateDescriptorSet(VkDescriptorSetLayout layout) {
		return allocateDescriptorSet(layout, "unnamed_set");
	}

	VkDescriptorSet DescriptorSetManager::allocateDescriptorSet(VkDescriptorSetLayout layout, const std::string& debugName) {
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_currentPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &layout;

		VkDescriptorSet descriptorSet;
		VkResult result = vkAllocateDescriptorSets(m_device.getDevice(), &allocInfo, &descriptorSet);
		
		// If allocation failed, create a new pool and try again
		if (result == VK_ERROR_OUT_OF_POOL_MEMORY) {
			AE_WARN("Descriptor pool out of memory, creating new pool");
			m_stats.failedAllocations++;
			createDescriptorPool();
			allocInfo.descriptorPool = m_currentPool;
			result = vkAllocateDescriptorSets(m_device.getDevice(), &allocInfo, &descriptorSet);
		}
		
	if (result != VK_SUCCESS) {
		m_stats.failedAllocations++;
		throw std::runtime_error("failed to allocate descriptor set: " + debugName);
	}

	// Track which pool this descriptor set belongs to
	m_setToPool[descriptorSet] = m_currentPool;
	m_stats.allocatedSets++;
	return descriptorSet;
	}

	void DescriptorSetManager::updateDescriptorSet(VkDescriptorSet descriptorSet, const std::vector<VkWriteDescriptorSet>& writes) {
		vkUpdateDescriptorSets(m_device.getDevice(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
	}

	void DescriptorSetManager::initializeSceneDescriptors(const std::vector<VkDescriptorSetLayoutBinding>& bindings, size_t swapchainImageCount) {
		// Create scene descriptor layout (set=0)
		m_sceneConfig.layout = getDescriptorSetLayout(bindings, "scene_layout");
		
		// Allocate descriptor sets for each swapchain image
		m_sceneConfig.descriptorSets.resize(swapchainImageCount);
		for (size_t i = 0; i < swapchainImageCount; ++i) {
			m_sceneConfig.descriptorSets[i] = allocateDescriptorSet(m_sceneConfig.layout, "scene_set_" + std::to_string(i));
		}
		
		m_sceneConfig.needsRebuild = false;
		AE_INFO("Scene descriptors initialized for {} swapchain images", swapchainImageCount);
	}

	VkDescriptorSet DescriptorSetManager::getSceneDescriptorSet(size_t imageIndex) const {
		if (imageIndex >= m_sceneConfig.descriptorSets.size()) {
			throw std::out_of_range("Scene descriptor set image index out of range");
		}
		return m_sceneConfig.descriptorSets[imageIndex];
	}

	void DescriptorSetManager::updateSceneDescriptorSets(size_t imageIndex, const std::vector<VkWriteDescriptorSet>& writes) {
		if (imageIndex >= m_sceneConfig.descriptorSets.size()) {
			throw std::out_of_range("Scene descriptor set image index out of range");
		}
		
		// Update the descriptor set for this image index
		std::vector<VkWriteDescriptorSet> updatedWrites = writes;
		for (auto& write : updatedWrites) {
			write.dstSet = m_sceneConfig.descriptorSets[imageIndex];
		}
		
		updateDescriptorSet(m_sceneConfig.descriptorSets[imageIndex], updatedWrites);
	}

void DescriptorSetManager::updateStats(VkDescriptorType type, bool success) {
	if (success) {
		m_stats.typeUsage[type]++;
	} else {
		m_stats.failedAllocations++;
	}
}

void DescriptorSetManager::freeDescriptorSet(VkDescriptorSet descriptorSet) {
	if (descriptorSet == VK_NULL_HANDLE) {
		return;
	}
	
	// Add to deferred deletion queue
	m_pendingDeletions.push_back(descriptorSet);
}

void DescriptorSetManager::freeDescriptorSets(const std::vector<VkDescriptorSet>& descriptorSets) {
	for (VkDescriptorSet set : descriptorSets) {
		if (set != VK_NULL_HANDLE) {
			m_pendingDeletions.push_back(set);
		}
	}
}

void DescriptorSetManager::processDeferredDeletions() {
	if (m_pendingDeletions.empty()) {
		return;
	}
	
	// Group descriptor sets by their pools for batch freeing
	std::unordered_map<VkDescriptorPool, std::vector<VkDescriptorSet>> poolGroups;
	
	for (VkDescriptorSet set : m_pendingDeletions) {
		auto it = m_setToPool.find(set);
		if (it != m_setToPool.end()) {
			poolGroups[it->second].push_back(set);
		} else {
			AE_WARN("Attempted to free descriptor set not tracked by manager");
		}
	}
	
	// Free descriptor sets in batches per pool
	for (const auto& [pool, sets] : poolGroups) {
		VkResult result = vkFreeDescriptorSets(m_device.getDevice(), pool, 
			static_cast<uint32_t>(sets.size()), sets.data());
		
		if (result == VK_SUCCESS) {
			m_stats.freedSets += static_cast<uint32_t>(sets.size());
			
			// Remove from tracking map
			for (VkDescriptorSet set : sets) {
				m_setToPool.erase(set);
			}
			
			AE_DEBUG("Freed {} descriptor sets from pool", sets.size());
		} else {
			AE_ERROR("Failed to free descriptor sets from pool (VkResult: {})", static_cast<int>(result));
		}
	}
	
	// Clear pending deletions
	m_pendingDeletions.clear();
	
	if (m_stats.freedSets > 0) {
		AE_DEBUG("Processed deferred deletions. Total freed sets: {}", m_stats.freedSets);
	}
}

void DescriptorSetManager::logStats() const {
	AE_INFO("DescriptorSetManager Statistics:");
	AE_INFO("  Total pools created: {}", m_stats.poolsCreated);
	AE_INFO("  Total sets capacity: {}", m_stats.totalSets);
	AE_INFO("  Allocated sets: {}", m_stats.allocatedSets);
	AE_INFO("  Freed sets: {}", m_stats.freedSets);
	AE_INFO("  Active sets: {}", m_stats.allocatedSets - m_stats.freedSets);
	AE_INFO("  Failed allocations: {}", m_stats.failedAllocations);
	
	if (!m_stats.typeUsage.empty()) {
		AE_INFO("  Descriptor type usage:");
		for (const auto& [type, count] : m_stats.typeUsage) {
			AE_INFO("    Type {}: {} descriptors", static_cast<int>(type), count);
		}
	}
}
}
