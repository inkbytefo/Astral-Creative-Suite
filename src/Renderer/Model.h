#pragma once

#include <vector>
#include <string>
#include "Renderer/MaterialShaderManager.h"
#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>

// Enable GLM hash functions for C++17
#ifndef GLM_FORCE_CXX17
#define GLM_FORCE_CXX17
#endif
#include <glm/gtx/hash.hpp>
#include <vulkan/vulkan.h>

// Forward declare ModelData to avoid circular dependency
struct ModelData;

namespace AstralEngine {
    // Forward declarations
    namespace Vulkan {
        class VulkanDevice;
        class VulkanBuffer;
    }
    class UnifiedMaterialInstance;
    struct ModelData;

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
	// Represents the GPU-side data of a 3D model.
	class Model {
	public:
		Model(Vulkan::VulkanDevice& device, std::unique_ptr<ModelData> modelData);
		~Model();

		void Bind(VkCommandBuffer commandBuffer);
		void Draw(VkCommandBuffer commandBuffer);
		void DrawSubMesh(VkCommandBuffer commandBuffer, size_t submeshIndex);

		uint32_t getIndexCount() const;
		size_t getSubmeshCount() const;
		        const SubMesh& getSubmesh(size_t index) const;
        std::shared_ptr<UnifiedMaterialInstance> getSubmeshMaterial(uint32_t index) const;


	private:
		void createVertexBuffers();
		void createIndexBuffers();

		Vulkan::VulkanDevice& m_device;
		
		// Geometry data (moved from ModelData)
		std::vector<Vertex> m_vertices;
		std::vector<uint32_t> m_indices;
		std::vector<SubMesh> m_subMeshes;
		
		// Vulkan resources
		std::unique_ptr<Vulkan::VulkanBuffer> m_vertexBuffer;
		std::unique_ptr<Vulkan::VulkanBuffer> m_indexBuffer;
	};
}