#include "MeshAsset.h"
#include "Core/Logger.h"
#include <vk_mem_alloc.h>
#include <cstring>

namespace AstralEngine {

    MeshAsset::MeshAsset(const std::string& path, VulkanR::VulkanDevice& device)
        : Asset(path), m_device(device), m_vertexBuffer(VK_NULL_HANDLE), 
          m_indexBuffer(VK_NULL_HANDLE), m_vertexBufferAllocation(VK_NULL_HANDLE),
          m_indexBufferAllocation(VK_NULL_HANDLE), m_isLoaded(false) {
        // Create a simple cube mesh as default
        // In a real implementation, this would load from a file
        m_vertices = {
            // Front face
            {{-0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}},
            {{ 0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 0.0f}},
            {{ 0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}},
            {{-0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 1.0f}},
            
            // Back face
            {{-0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.0f, 0.0f}},
            {{-0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 0.0f}},
            {{ 0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 1.0f}},
            {{ 0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.0f, 1.0f}},
            
            // Top face
            {{-0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 0.0f}},
            {{-0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {1.0f, 0.0f}},
            {{ 0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {1.0f, 1.0f}},
            {{ 0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 1.0f}},
            
            // Bottom face
            {{-0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {0.0f, 0.0f}},
            {{ 0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {1.0f, 0.0f}},
            {{ 0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {1.0f, 1.0f}},
            {{-0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {0.0f, 1.0f}},
            
            // Right face
            {{ 0.5f, -0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}},
            {{ 0.5f,  0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}},
            {{ 0.5f,  0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}},
            {{ 0.5f, -0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}},
            
            // Left face
            {{-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}},
            {{-0.5f, -0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}},
            {{-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}},
            {{-0.5f,  0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}},
        };
        
        m_indices = {
            0,  1,  2,  2,  3,  0,  // Front
            4,  5,  6,  6,  7,  4,  // Back
            8,  9,  10, 10, 11, 8,  // Top
            12, 13, 14, 14, 15, 12, // Bottom
            16, 17, 18, 18, 19, 16, // Right
            20, 21, 22, 22, 23, 20  // Left
        };
    }

    MeshAsset::~MeshAsset() {
        cleanup();
    }

    bool MeshAsset::load() {
        // In a real implementation, this would load from a file
        // For now, we're using the default cube mesh created in the constructor
        
        createVertexBuffer();
        createIndexBuffer();
        
        m_isLoaded = true;
        AE_INFO("Mesh asset loaded: {}", getPath());
        return true;
    }

    void MeshAsset::unload() {
        cleanup();
        m_isLoaded = false;
        AE_INFO("Mesh asset unloaded: {}", getPath());
    }

    bool MeshAsset::isLoaded() const {
        return m_isLoaded;
    }

    void MeshAsset::createVertexBuffer() {
        VkDeviceSize bufferSize = sizeof(m_vertices[0]) * m_vertices.size();

        VkBuffer stagingBuffer;
        VmaAllocation stagingBufferAllocation;
        
        VmaAllocationCreateInfo stagingAllocInfo = {};
        stagingAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

        VkBufferCreateInfo stagingBufferInfo = {};
        stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        stagingBufferInfo.size = bufferSize;
        stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vmaCreateBuffer(m_device.getAllocator(), &stagingBufferInfo, &stagingAllocInfo, 
                           &stagingBuffer, &stagingBufferAllocation, nullptr) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create staging buffer for vertex buffer!");
        }

        void* data;
        vmaMapMemory(m_device.getAllocator(), stagingBufferAllocation, &data);
        memcpy(data, m_vertices.data(), (size_t)bufferSize);
        vmaUnmapMemory(m_device.getAllocator(), stagingBufferAllocation);

        VmaAllocationCreateInfo vertexAllocInfo = {};
        vertexAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        VkBufferCreateInfo vertexBufferInfo = {};
        vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        vertexBufferInfo.size = bufferSize;
        vertexBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        vertexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vmaCreateBuffer(m_device.getAllocator(), &vertexBufferInfo, &vertexAllocInfo, 
                           &m_vertexBuffer, &m_vertexBufferAllocation, nullptr) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create vertex buffer!");
        }

        m_device.copyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);

        vmaDestroyBuffer(m_device.getAllocator(), stagingBuffer, stagingBufferAllocation);
    }

    void MeshAsset::createIndexBuffer() {
        VkDeviceSize bufferSize = sizeof(m_indices[0]) * m_indices.size();

        VkBuffer stagingBuffer;
        VmaAllocation stagingBufferAllocation;
        
        VmaAllocationCreateInfo stagingAllocInfo = {};
        stagingAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

        VkBufferCreateInfo stagingBufferInfo = {};
        stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        stagingBufferInfo.size = bufferSize;
        stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vmaCreateBuffer(m_device.getAllocator(), &stagingBufferInfo, &stagingAllocInfo, 
                           &stagingBuffer, &stagingBufferAllocation, nullptr) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create staging buffer for index buffer!");
        }

        void* data;
        vmaMapMemory(m_device.getAllocator(), stagingBufferAllocation, &data);
        memcpy(data, m_indices.data(), (size_t)bufferSize);
        vmaUnmapMemory(m_device.getAllocator(), stagingBufferAllocation);

        VmaAllocationCreateInfo indexAllocInfo = {};
        indexAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        VkBufferCreateInfo indexBufferInfo = {};
        indexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        indexBufferInfo.size = bufferSize;
        indexBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        indexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vmaCreateBuffer(m_device.getAllocator(), &indexBufferInfo, &indexAllocInfo, 
                           &m_indexBuffer, &m_indexBufferAllocation, nullptr) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create index buffer!");
        }

        m_device.copyBuffer(stagingBuffer, m_indexBuffer, bufferSize);

        vmaDestroyBuffer(m_device.getAllocator(), stagingBuffer, stagingBufferAllocation);
    }

    void MeshAsset::cleanup() {
        if (m_indexBuffer != VK_NULL_HANDLE) {
            vmaDestroyBuffer(m_device.getAllocator(), m_indexBuffer, m_indexBufferAllocation);
            m_indexBuffer = VK_NULL_HANDLE;
            m_indexBufferAllocation = VK_NULL_HANDLE;
        }
        
        if (m_vertexBuffer != VK_NULL_HANDLE) {
            vmaDestroyBuffer(m_device.getAllocator(), m_vertexBuffer, m_vertexBufferAllocation);
            m_vertexBuffer = VK_NULL_HANDLE;
            m_vertexBufferAllocation = VK_NULL_HANDLE;
        }
    }

}