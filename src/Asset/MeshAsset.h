#pragma once

#include "Asset/Asset.h"
#include "Renderer/VulkanR/VulkanDevice.h"
#include <vector>
#include <memory>

namespace AstralEngine {

    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texCoord;
        
        static VkVertexInputBindingDescription getBindingDescription() {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(Vertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            
            return bindingDescription;
        }
        
        static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
            std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
            
            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(Vertex, position);
            
            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(Vertex, normal);
            
            attributeDescriptions[2].binding = 0;
            attributeDescriptions[2].location = 2;
            attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[2].offset = offsetof(Vertex, texCoord);
            
            return attributeDescriptions;
        }
    };

    class MeshAsset : public Asset {
    public:
        MeshAsset(const std::string& path, VulkanR::VulkanDevice& device);
        virtual ~MeshAsset();
        
        virtual bool load() override;
        virtual void unload() override;
        virtual bool isLoaded() const override;
        
        const std::vector<Vertex>& getVertices() const { return m_vertices; }
        const std::vector<uint32_t>& getIndices() const { return m_indices; }
        
        VkBuffer getVertexBuffer() const { return m_vertexBuffer; }
        VkBuffer getIndexBuffer() const { return m_indexBuffer; }
        VkDeviceSize getVertexBufferOffset() const { return 0; }
        VkDeviceSize getIndexBufferOffset() const { return 0; }
        
    private:
        void createVertexBuffer();
        void createIndexBuffer();
        void cleanup();
        
        VulkanR::VulkanDevice& m_device;
        std::vector<Vertex> m_vertices;
        std::vector<uint32_t> m_indices;
        
        VkBuffer m_vertexBuffer;
        VkBuffer m_indexBuffer;
        VmaAllocation m_vertexBufferAllocation;
        VmaAllocation m_indexBufferAllocation;
        
        bool m_isLoaded;
    };

}