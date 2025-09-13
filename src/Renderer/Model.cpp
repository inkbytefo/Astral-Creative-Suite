#include "Renderer/Model.h"
#include "Asset/ModelLoader.h" // For ModelData
#include "Core/Logger.h"
#include "Core/MemoryManager.h"
#include "Renderer/VulkanR/VulkanDevice.h"
#include "Renderer/VulkanR/VulkanBuffer.h"

#include <stdexcept>

namespace AstralEngine {

    Model::Model(Vulkan::VulkanDevice& device, std::unique_ptr<ModelData> modelData)
        : m_device(device) {

        if (!modelData) {
            throw std::runtime_error("Cannot create Model from null modelData");
        }

        // Move data from ModelData to this Model instance
        m_vertices = std::move(modelData->vertices);
        m_indices = std::move(modelData->indices);
        m_subMeshes = std::move(modelData->subMeshes);

        if (!m_vertices.empty()) {
            createVertexBuffers();
        }
        if (!m_indices.empty()) {
            createIndexBuffers();
        }

        AE_DEBUG("Model GPU resources created. Vertices: {}, Indices: {}", m_vertices.size(), m_indices.size());
    }

    Model::~Model() = default;

    void Model::createVertexBuffers() {
        VkDeviceSize bufferSize = sizeof(m_vertices[0]) * m_vertices.size();
        if (bufferSize == 0) return;

        Vulkan::VulkanBuffer stagingBuffer(m_device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

        void* data;
        vmaMapMemory(m_device.getAllocator(), stagingBuffer.getAllocation(), &data);
        memcpy(data, m_vertices.data(), (size_t)bufferSize);
        vmaUnmapMemory(m_device.getAllocator(), stagingBuffer.getAllocation());

        m_vertexBuffer = std::make_unique<Vulkan::VulkanBuffer>(m_device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

        m_device.copyBuffer(stagingBuffer.getBuffer(), m_vertexBuffer->getBuffer(), bufferSize);
    }

    void Model::createIndexBuffers() {
        VkDeviceSize bufferSize = sizeof(m_indices[0]) * m_indices.size();
        if (bufferSize == 0) return;

        Vulkan::VulkanBuffer stagingBuffer(m_device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

        void* data;
        vmaMapMemory(m_device.getAllocator(), stagingBuffer.getAllocation(), &data);
        memcpy(data, m_indices.data(), (size_t)bufferSize);
        vmaUnmapMemory(m_device.getAllocator(), stagingBuffer.getAllocation());

        m_indexBuffer = std::make_unique<Vulkan::VulkanBuffer>(m_device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

        m_device.copyBuffer(stagingBuffer.getBuffer(), m_indexBuffer->getBuffer(), bufferSize);
    }

    void Model::Bind(VkCommandBuffer commandBuffer) {
        if (m_vertexBuffer && m_indexBuffer) {
            VkBuffer buffers[] = {m_vertexBuffer->getBuffer()};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
            vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
        }
    }

    void Model::Draw(VkCommandBuffer commandBuffer) {
        if (m_indexBuffer && !m_indices.empty()) {
            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);
        }
    }

    void Model::DrawSubMesh(VkCommandBuffer commandBuffer, size_t submeshIndex) {
        if (submeshIndex >= m_subMeshes.size()) return;
        const auto& sm = m_subMeshes[submeshIndex];
        vkCmdDrawIndexed(commandBuffer, sm.indexCount, 1, sm.indexOffset, 0, 0);
    }

    uint32_t Model::getIndexCount() const {
        return static_cast<uint32_t>(m_indices.size());
    }

    size_t Model::getSubmeshCount() const {
        return m_subMeshes.size();
    }

    const SubMesh& Model::getSubmesh(size_t index) const {
        if (index >= m_subMeshes.size()) {
            throw std::out_of_range("Submesh index out of range");
        }
        return m_subMeshes[index];
    }

    std::shared_ptr<UnifiedMaterialInstance> Model::getSubmeshMaterial(uint32_t index) const {
        if (index >= m_subMeshes.size() || !m_subMeshes[index].material) {
            // Return a default material or nullptr if you have one
            return nullptr;
        }
        return m_subMeshes[index].material;
    }

    // Vertex struct method implementations
    std::vector<VkVertexInputBindingDescription> Vertex::getBindingDescriptions() {
        std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
        bindingDescriptions[0].binding = 0;
        bindingDescriptions[0].stride = sizeof(Vertex);
        bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescriptions;
    }

    std::vector<VkVertexInputAttributeDescription> Vertex::getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions(5);

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, position);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, normal);

        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[3].offset = offsetof(Vertex, texCoord);

        attributeDescriptions[4].binding = 0;
        attributeDescriptions[4].location = 4;
        attributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[4].offset = offsetof(Vertex, tangent);

        return attributeDescriptions;
    }
}