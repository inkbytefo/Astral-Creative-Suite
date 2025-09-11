#pragma once

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <atomic>
#include <filesystem>
#include <glm/glm.hpp>

// Enable GLM hash functions for C++17
#ifndef GLM_FORCE_CXX17
#define GLM_FORCE_CXX17
#endif
#include <glm/gtx/hash.hpp>
#include <vulkan/vulkan.h>
#include "Core/AssetDependency.h"

namespace AstralEngine {
    // Forward declarations
    namespace Vulkan {
        class VulkanDevice;
        class VulkanBuffer;
    }
    class UnifiedMaterialInstance;
    class DependencyGraph;
    class Texture;
    class MaterialLibrary;

	struct Vertex {
		glm::vec3 position;
		glm::vec3 color;
		glm::vec3 normal;
		glm::vec2 texCoord;
		glm::vec3 tangent = {0.0f, 0.0f, 0.0f};
		glm::vec3 bitangent = {0.0f, 0.0f, 0.0f};

		bool operator==(const Vertex& other) const {
			return position == other.position && color == other.color && 
			       normal == other.normal && texCoord == other.texCoord &&
			       tangent == other.tangent && bitangent == other.bitangent;
		}

		static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
		static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
	};

	/**
	 * @brief Represents a submesh with its own material and draw range
	 */
	struct SubMesh {
		std::string name;
		std::string materialName; // Reference to material by name
		uint32_t indexOffset = 0;
		uint32_t indexCount = 0;
		std::shared_ptr<UnifiedMaterialInstance> material; // Loaded material instance
		
		SubMesh() = default;
		SubMesh(const std::string& n, const std::string& matName, uint32_t offset, uint32_t count)
			: name(n), materialName(matName), indexOffset(offset), indexCount(count) {}
	};
}

namespace std {
    template<> struct hash<AstralEngine::Vertex> {
        size_t operator()(AstralEngine::Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.position) ^
                   (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
                   (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}

namespace AstralEngine {
	class Model {
	public:
		using LoadCompleteCallback = std::function<void(bool success, const std::string& error)>;

		Model(Vulkan::VulkanDevice& device, const std::string& filepath);
		~Model();

		/**
		 * @brief Load model with all dependencies asynchronously
		 */
		void loadAsync(LoadCompleteCallback callback);

		/**
		 * @brief Check if model and all dependencies are fully loaded
		 */
		bool isFullyLoaded() const;

		/**
		 * @brief Get loading progress (0.0 to 1.0)
		 */
		float getLoadingProgress() const;

		void Bind(VkCommandBuffer commandBuffer);
		void Draw(VkCommandBuffer commandBuffer);
		
		/**
		 * @brief Draw specific submesh
		 */
		void DrawSubMesh(VkCommandBuffer commandBuffer, size_t submeshIndex);

		uint32_t GetIndexCount() const { return static_cast<uint32_t>(indices.size()); }
		const std::vector<std::string>& getTextureDependencies() const { return m_textureDependencies; }
		
		/**
		 * @brief Get all submeshes
		 */
		const std::vector<SubMesh>& getSubMeshes() const { return m_subMeshes; }
		
		/**
		 * @brief Get material by name (after loading is complete)
		 */
		std::shared_ptr<UnifiedMaterialInstance> getMaterial(const std::string& name) const;
		
		/**
		 * @brief Get default material for submesh (fallback)
		 */
		std::shared_ptr<UnifiedMaterialInstance> getDefaultMaterial() const;
		
		/**
		 * @brief Get all materials in this model
		 */
		const std::unordered_map<std::string, std::shared_ptr<UnifiedMaterialInstance>>& getMaterials() const;
		
		/**
		 * @brief Get material for a specific submesh by index (fallback-safe)
		 */
		std::shared_ptr<UnifiedMaterialInstance> getSubmeshMaterial(uint32_t index) const;
		
		/**
		 * @brief Check if materials are ready for rendering
		 */
		bool areMaterialsReady() const;
		
		/**
		 * @brief Get material loading progress (0.0 to 1.0)
		 */
		float getMaterialLoadingProgress() const;
		
		/**
		 * @brief Get number of submeshes in this model
		 */
		size_t getSubmeshCount() const;
		
		/**
		 * @brief Get submesh by index
		 */
		const SubMesh& getSubmesh(size_t index) const;

	private:
		void loadModel(const std::string& filepath);
		void createVertexBuffers();
		void createIndexBuffers();
		void generateTangents(); // Calculate tangent/bitangent vectors
		void loadMaterialsFromMTL(const std::string& mtlPath, const std::string& baseDir);
		void createDefaultMaterial();
		
		// Enhanced material loading methods
		void parseMTLFiles(const std::vector<std::string>& mtlFiles);
		void collectTexturePathsFromMaterials();
		void createLoadedMaterialInstances();
		void createUnloadedMaterialInstances();
		void bindMaterialsToSubmeshes();
		std::string normalizeTexturePath(const std::string& basePath, const std::string& relativePath) const;

		Vulkan::VulkanDevice& m_device;
		std::string m_filepath;
		std::string m_baseDirectory;
		
		// Geometry data
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		std::vector<SubMesh> m_subMeshes;
		
		// Material and dependency management
		std::vector<std::string> m_textureDependencies; // Legacy - to be phased out
		
		// Enhanced material management
		std::unordered_map<std::string, MaterialLibrary::MaterialInfo> m_materialInfos; // keyed by material name
		std::unordered_map<std::string, std::shared_ptr<UnifiedMaterialInstance>> m_materials; // keyed by material name
		std::unordered_map<std::string, std::string> m_materialNameToBaseDir; // handle multiple MTLs
		std::unordered_map<std::string, std::shared_ptr<Texture>> m_loadedTextures; // keyed by normalized absolute path
		std::unordered_set<std::string> m_texturePaths; // absolute, normalized
		
		std::shared_ptr<UnifiedMaterialInstance> m_defaultMaterial;
		std::unique_ptr<DependencyGraph> m_dependencyGraph;
		bool m_loadingComplete = false;
		std::atomic<bool> m_materialsReady{false};
		std::string m_modelDirectory; // model base path
		
		// Vulkan resources
		std::unique_ptr<Vulkan::VulkanBuffer> m_vertexBuffer;
		std::unique_ptr<Vulkan::VulkanBuffer> m_indexBuffer;
	};
}