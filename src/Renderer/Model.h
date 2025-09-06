#pragma once

#include "Renderer/Vulkan/VulkanBuffer.h"
#include "vk_mem_alloc.h"
#include <vector>
#include <string>

// GLM'i matematik için kullanacağız (projeye eklenmesi gerekecek)
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace Astral {
    struct Vertex {
        glm::vec3 position;
        glm::vec3 color;

        static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
    };

    class Model {
    public:
        Model(Vulkan::VulkanDevice& device, VmaAllocator allocator, const std::string& filepath);
        ~Model();

        Model(const Model&) = delete;
        Model& operator=(const Model&) = delete;

        void Bind(VkCommandBuffer commandBuffer);
        uint32_t GetIndexCount() const { return m_indexCount; }

    private:
        void loadModel(const std::string& filepath);
        void createVertexBuffers(const std::vector<Vertex>& vertices);
        void createIndexBuffers(const std::vector<uint32_t>& indices);

        Vulkan::VulkanDevice& m_device;
        VmaAllocator m_allocator;
        Vulkan::VulkanBuffer m_vertexBuffer;
        Vulkan::VulkanBuffer m_indexBuffer;
        uint32_t m_indexCount = 0;
    };
}
