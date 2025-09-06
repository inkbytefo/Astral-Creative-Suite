#include "Model.h"
#include "Core/Logger.h"
#include "Vulkan/VulkanDevice.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include <stdexcept>
#include <unordered_map>

// GLM için hash fonksiyonu, unordered_map'te anahtar olarak kullanabilmek için
namespace std {
    template<> struct hash<Astral::Vertex> {
        size_t operator()(Astral::Vertex const& vertex) const {
            size_t seed = 0;
            // Konum ve renk bileşenlerini hash'lemek için
            hash<float> hasher;
            seed ^= hasher(vertex.position.x) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= hasher(vertex.position.y) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= hasher(vertex.position.z) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= hasher(vertex.color.r) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= hasher(vertex.color.g) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= hasher(vertex.color.b) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            return seed;
        }
    };
}

namespace Astral {

    // --- Vertex Struct Metotları ---

    std::vector<VkVertexInputBindingDescription> Vertex::getBindingDescriptions() {
        std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
        bindingDescriptions[0].binding = 0;
        bindingDescriptions[0].stride = sizeof(Vertex);
        bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescriptions;
    }

    std::vector<VkVertexInputAttributeDescription> Vertex::getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);
        
        // Konum (position)
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, position);

        // Renk (color)
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        return attributeDescriptions;
    }

    bool operator==(const Vertex& a, const Vertex& b) {
        return a.position == b.position && a.color == b.color;
    }

    // --- Model Sınıfı Metotları ---

    Model::Model(Vulkan::VulkanDevice& device, VmaAllocator allocator, const std::string& filepath)
        : m_device(device), m_allocator(allocator) {
        loadModel(filepath);
    }

    Model::~Model() {
        vmaDestroyBuffer(m_allocator, m_vertexBuffer.buffer, m_vertexBuffer.allocation);
        vmaDestroyBuffer(m_allocator, m_indexBuffer.buffer, m_indexBuffer.allocation);
    }

    void Model::loadModel(const std::string& filepath) {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        // --- DÜZELTME BAŞLANGICI ---
        // LoadObj çağrısından &err parametresini kaldır.
        // Header'daki imzada &err parametresi yok. Hata mesajları &warn parametresine yazılır.
        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, filepath.c_str())) {
            throw std::runtime_error(warn);
        }
        // --- DÜZELTME SONU ---

        if (!warn.empty()) {
            ASTRAL_LOG_WARN("TinyObjLoader: %s", warn.c_str());
        }

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        std::unordered_map<Vertex, uint32_t> uniqueVertices{};

        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                Vertex vertex{};

                vertex.position = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };

                // .obj formatında vertex rengi genellikle olmaz.
                // Şimdilik pozisyona dayalı basit bir renk atayalım.
                vertex.color = {
                    (vertex.position.x + 1.0f) / 2.0f,
                    (vertex.position.y + 1.0f) / 2.0f,
                    (vertex.position.z + 1.0f) / 2.0f
                };

                if (uniqueVertices.count(vertex) == 0) {
                    uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }
                indices.push_back(uniqueVertices[vertex]);
            }
        }
        
        m_indexCount = static_cast<uint32_t>(indices.size());
        createVertexBuffers(vertices);
        createIndexBuffers(indices);

        ASTRAL_LOG_INFO("%s modelinden %zu vertex ve %zu index yüklendi.", filepath.c_str(), vertices.size(), indices.size());
    }

    void Model::createVertexBuffers(const std::vector<Vertex>& vertices) {
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        // Staging buffer (CPU tarafında)
        VkBuffer stagingBuffer;
        VmaAllocation stagingAllocation;
        
        VkBufferCreateInfo stagingBufferInfo{};
        stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        stagingBufferInfo.size = bufferSize;
        stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        VmaAllocationCreateInfo stagingAllocInfo{};
        stagingAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

        vmaCreateBuffer(m_allocator, &stagingBufferInfo, &stagingAllocInfo, &stagingBuffer, &stagingAllocation, nullptr);

        // Veriyi staging buffer'a kopyala
        void* data;
        vmaMapMemory(m_allocator, stagingAllocation, &data);
        memcpy(data, vertices.data(), (size_t)bufferSize);
        vmaUnmapMemory(m_allocator, stagingAllocation);

        // Asıl vertex buffer (GPU tarafında)
        VkBufferCreateInfo vertexBufferInfo{};
        vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        vertexBufferInfo.size = bufferSize;
        vertexBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

        VmaAllocationCreateInfo vertexAllocInfo{};
        vertexAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        vmaCreateBuffer(m_allocator, &vertexBufferInfo, &vertexAllocInfo, &m_vertexBuffer.buffer, &m_vertexBuffer.allocation, nullptr);

        // Staging buffer'dan vertex buffer'a kopyala
        m_device.CopyBuffer(stagingBuffer, m_vertexBuffer.buffer, bufferSize);

        // Staging buffer'ı temizle
        vmaDestroyBuffer(m_allocator, stagingBuffer, stagingAllocation);
    }

    void Model::createIndexBuffers(const std::vector<uint32_t>& indices) {
        VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

        VkBuffer stagingBuffer;
        VmaAllocation stagingAllocation;

        VkBufferCreateInfo stagingBufferInfo{};
        stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        stagingBufferInfo.size = bufferSize;
        stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        VmaAllocationCreateInfo stagingAllocInfo{};
        stagingAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

        vmaCreateBuffer(m_allocator, &stagingBufferInfo, &stagingAllocInfo, &stagingBuffer, &stagingAllocation, nullptr);

        void* data;
        vmaMapMemory(m_allocator, stagingAllocation, &data);
        memcpy(data, indices.data(), (size_t)bufferSize);
        vmaUnmapMemory(m_allocator, stagingAllocation);

        VkBufferCreateInfo indexBufferInfo{};
        indexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        indexBufferInfo.size = bufferSize;
        indexBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

        VmaAllocationCreateInfo indexAllocInfo{};
        indexAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        vmaCreateBuffer(m_allocator, &indexBufferInfo, &indexAllocInfo, &m_indexBuffer.buffer, &m_indexBuffer.allocation, nullptr);

        m_device.CopyBuffer(stagingBuffer, m_indexBuffer.buffer, bufferSize);

        vmaDestroyBuffer(m_allocator, stagingBuffer, stagingAllocation);
    }

    void Model::Bind(VkCommandBuffer commandBuffer) {
        VkBuffer buffers[] = {m_vertexBuffer.buffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    }
}
