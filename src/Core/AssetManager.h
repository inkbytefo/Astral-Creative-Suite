#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <future>
#include <mutex>
#include "Core/ThreadPool.h"

// Forward declarations
namespace AstralEngine {
    class Model;
    class Texture;
    class UnifiedMaterialInstance;
    namespace Vulkan {
        class VulkanDevice;
    }
}

namespace AstralEngine {
	class AssetManager {
	public:
		static void init();
		static void shutdown();

		static std::future<std::shared_ptr<Model>> loadModelAsync(Vulkan::VulkanDevice& device, const std::string& filepath);
		static std::future<std::shared_ptr<Texture>> loadTextureAsync(Vulkan::VulkanDevice& device, const std::string& filepath);
		static std::shared_ptr<UnifiedMaterialInstance> loadMaterial(Vulkan::VulkanDevice& device, const std::string& filepath);

	private:
		static std::unique_ptr<ThreadPool> m_threadPool;
		static std::unordered_map<std::string, std::shared_ptr<Model>> m_modelCache;
		static std::unordered_map<std::string, std::shared_ptr<Texture>> m_textureCache;
		static std::unordered_map<std::string, std::shared_ptr<UnifiedMaterialInstance>> m_materialCache;
		
		// Mutexes for thread safety
		static std::mutex m_modelCacheMutex;
		static std::mutex m_textureCacheMutex;
		static std::mutex m_materialCacheMutex;
	};
}