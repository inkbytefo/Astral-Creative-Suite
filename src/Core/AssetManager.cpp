#include "AssetManager.h"
#include "AssetLocator.h"
#include "Renderer/Model.h"
#include "Renderer/Texture.h"
#include "Renderer/UnifiedMaterial.h"
#include "Renderer/Shader.h"
#include "Renderer/Vulkan/VulkanDevice.h"

namespace AstralEngine {
	std::unique_ptr<ThreadPool> AssetManager::m_threadPool;
	std::unordered_map<std::string, std::shared_ptr<Model>> AssetManager::m_modelCache;
	std::unordered_map<std::string, std::shared_ptr<Texture>> AssetManager::m_textureCache;
	std::unordered_map<std::string, std::shared_ptr<UnifiedMaterialInstance>> AssetManager::m_materialCache;
	
	// Initialize mutexes
	std::mutex AssetManager::m_modelCacheMutex;
	std::mutex AssetManager::m_textureCacheMutex;
	std::mutex AssetManager::m_materialCacheMutex;

	void AssetManager::init() {
		m_threadPool = std::make_unique<ThreadPool>(4);
	}

	void AssetManager::shutdown() {
		m_threadPool.reset();
	}

	std::future<std::shared_ptr<Model>> AssetManager::loadModelAsync(Vulkan::VulkanDevice& device, const std::string& filepath) {
		// --- DÜZELTME: Namespace ekle ---
		Vulkan::VulkanDevice* devicePtr = &device;
        return m_threadPool->enqueue([devicePtr, filepath] {
            // Resolve asset path using AssetLocator
            std::string resolvedPath = AssetLocator::getInstance().resolveAssetPath(filepath);
            
            // Thread-safe access to model cache
            {
                std::lock_guard<std::mutex> lock(m_modelCacheMutex);
                if (m_modelCache.find(resolvedPath) != m_modelCache.end()) {
                    return m_modelCache[resolvedPath];
                }
            }
            
            // Load model outside of lock to minimize lock time
            auto model = std::make_shared<Model>(*devicePtr, resolvedPath);
			
            // Store in cache with lock
            {
                std::lock_guard<std::mutex> lock(m_modelCacheMutex);
                m_modelCache[resolvedPath] = model;
            }
			
			// NEW: Automatically kick off loading of texture dependencies
			try {
				const auto& deps = model->getTextureDependencies();
				for (const auto& texPath : deps) {
					// Fire and forget texture loads; cache will deduplicate
					(void)loadTextureAsync(*devicePtr, texPath);
				}
			} catch (...) {
				// Ignore dependency loading errors here; individual texture loads will log
			}
			
			return model;
		});
	}

	std::future<std::shared_ptr<Texture>> AssetManager::loadTextureAsync(Vulkan::VulkanDevice& device, const std::string& filepath) {
		// --- DÜZELTME: Namespace ekle ---
		Vulkan::VulkanDevice* devicePtr = &device;
        return m_threadPool->enqueue([devicePtr, filepath] {
            // Resolve asset path using AssetLocator
            std::string resolvedPath = AssetLocator::getInstance().resolveAssetPath(filepath);
            
            // Thread-safe access to texture cache
            {
                std::lock_guard<std::mutex> lock(m_textureCacheMutex);
                if (m_textureCache.find(resolvedPath) != m_textureCache.end()) {
                    return m_textureCache[resolvedPath];
                }
            }
            
            // Load texture outside of lock to minimize lock time
            auto texture = std::make_shared<Texture>(*devicePtr, resolvedPath);
			
            // Store in cache with lock
            {
                std::lock_guard<std::mutex> lock(m_textureCacheMutex);
                m_textureCache[resolvedPath] = texture;
            }
			
			return texture;
		});
	}

	std::shared_ptr<UnifiedMaterialInstance> AssetManager::loadMaterial(Vulkan::VulkanDevice& device, const std::string& filepath) {
		// Thread-safe access to material cache
		{
			std::lock_guard<std::mutex> lock(m_materialCacheMutex);
			if (m_materialCache.find(filepath) != m_materialCache.end()) {
				return m_materialCache[filepath];
			}
		}
		
		// For now, just create a default unified material
		auto material = std::make_shared<UnifiedMaterialInstance>();
                *material = UnifiedMaterialInstance::createDefault();
		
		// Store in cache with lock
		{
			std::lock_guard<std::mutex> lock(m_materialCacheMutex);
			m_materialCache[filepath] = material;
		}
		
		return material;
	}
}
