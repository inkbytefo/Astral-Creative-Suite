#include "Render3DSystem.h"
#include "Core/Logger.h"
#include <vk_mem_alloc.h>
#include <array>

namespace AstralEngine::D3 {

    Render3DSystem::Render3DSystem(VulkanR::VulkanDevice& device)
        : m_device(device), m_pipelineLayout(VK_NULL_HANDLE), 
          m_graphicsPipeline(VK_NULL_HANDLE), m_descriptorSetLayout(VK_NULL_HANDLE),
          m_descriptorPool(VK_NULL_HANDLE) {
        createDescriptorSetLayout();
        createPipeline();
        createDescriptorPool();
        createUniformBuffers();
        createDescriptorSets();
    }

    Render3DSystem::~Render3DSystem() {
        // Cleanup in reverse order
        for (size_t i = 0; i < m_uniformBufferAllocations.size(); i++) {
            vmaUnmapMemory(m_device.getAllocator(), m_uniformBufferAllocations[i]);
            vmaDestroyBuffer(m_device.getAllocator(), m_uniformBuffers[i], m_uniformBufferAllocations[i]);
        }
        
        if (m_descriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(m_device.getDevice(), m_descriptorPool, nullptr);
        }
        
        if (m_descriptorSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(m_device.getDevice(), m_descriptorSetLayout, nullptr);
        }
        
        if (m_pipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(m_device.getDevice(), m_pipelineLayout, nullptr);
        }
        
        if (m_graphicsPipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(m_device.getDevice(), m_graphicsPipeline, nullptr);
        }
    }

    void Render3DSystem::render(VkCommandBuffer commandBuffer, const ECS::Scene& scene) {
        // Update uniform buffers with current scene data
        updateUniformBuffers(scene);
        
        // Bind the graphics pipeline
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
        
        // TODO: Bind descriptor sets
        // TODO: Bind vertex and index buffers
        // TODO: Draw commands
        
        // This is a placeholder implementation
        // In a real implementation, we would:
        // 1. Iterate through all entities with Mesh3D components
        // 2. For each entity:
        //    a. Bind the appropriate descriptor set
        //    b. Bind the vertex and index buffers
        //    c. Issue draw commands
    }

    void Render3DSystem::createPipeline() {
        // This is a simplified pipeline creation
        // In a real implementation, this would be much more complex
        
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;

        if (vkCreatePipelineLayout(m_device.getDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create pipeline layout!");
        }
        
        // For now, we'll just create an empty pipeline
        // In a real implementation, we would load shaders and create a full graphics pipeline
        m_graphicsPipeline = VK_NULL_HANDLE;
    }

    void Render3DSystem::createDescriptorSetLayout() {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uboLayoutBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &uboLayoutBinding;

        if (vkCreateDescriptorSetLayout(m_device.getDevice(), &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor set layout!");
        }
    }

    void Render3DSystem::createDescriptorPool() {
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = static_cast<uint32_t>(m_uniformBuffers.size());

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = static_cast<uint32_t>(m_uniformBuffers.size());

        if (vkCreateDescriptorPool(m_device.getDevice(), &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor pool!");
        }
    }

    void Render3DSystem::createDescriptorSets() {
        std::vector<VkDescriptorSetLayout> layouts(m_uniformBuffers.size(), m_descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(m_uniformBuffers.size());
        allocInfo.pSetLayouts = layouts.data();

        m_descriptorSets.resize(m_uniformBuffers.size());
        if (vkAllocateDescriptorSets(m_device.getDevice(), &allocInfo, m_descriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate descriptor sets!");
        }

        for (size_t i = 0; i < m_uniformBuffers.size(); i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = m_uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = m_descriptorSets[i];
            descriptorWrite.dstBinding = 0;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pBufferInfo = &bufferInfo;
            descriptorWrite.pImageInfo = nullptr;
            descriptorWrite.pTexelBufferView = nullptr;

            vkUpdateDescriptorSets(m_device.getDevice(), 1, &descriptorWrite, 0, nullptr);
        }
    }

    void Render3DSystem::createUniformBuffers() {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        m_uniformBuffers.resize(2); // Double buffering
        m_uniformBufferAllocations.resize(2);
        m_uniformBufferMapped.resize(2);

        for (size_t i = 0; i < 2; i++) {
            VmaAllocationCreateInfo allocInfo = {};
            allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

            VkBufferCreateInfo bufferInfo = {};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = bufferSize;
            bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            if (vmaCreateBuffer(m_device.getAllocator(), &bufferInfo, &allocInfo, 
                               &m_uniformBuffers[i], &m_uniformBufferAllocations[i], nullptr) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create uniform buffer!");
            }

            vmaMapMemory(m_device.getAllocator(), m_uniformBufferAllocations[i], &m_uniformBufferMapped[i]);
        }
    }

    void Render3DSystem::updateUniformBuffers(const ECS::Scene& scene) {
        // This is a placeholder implementation
        // In a real implementation, we would:
        // 1. Find the camera entity in the scene
        // 2. Calculate the view and projection matrices
        // 3. For each mesh entity:
        //    a. Calculate the model matrix
        //    b. Update the uniform buffer with the MVP matrices
        
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        UniformBufferObject ubo{};
        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f), 16.0f / 9.0f, 0.1f, 10.0f);
        ubo.proj[1][1] *= -1; // Flip Y coordinate for Vulkan

        memcpy(m_uniformBufferMapped[0], &ubo, sizeof(ubo));
    }

}