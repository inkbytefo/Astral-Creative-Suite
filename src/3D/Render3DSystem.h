#pragma once

#include "3D/Components3D.h"
#include "ECS/Scene.h"
#include "Renderer/VulkanR/VulkanDevice.h"
#include <vector>

namespace AstralEngine::D3 {

    class Render3DSystem {
    public:
        Render3DSystem(VulkanR::VulkanDevice& device);
        ~Render3DSystem();

        void render(VkCommandBuffer commandBuffer, const ECS::Scene& scene);

    private:
        void createPipeline();
        void createDescriptorSetLayout();
        void createDescriptorPool();
        void createDescriptorSets();
        void createUniformBuffers();
        void updateUniformBuffers(const ECS::Scene& scene);

        VulkanR::VulkanDevice& m_device;
        
        // Pipeline objects
        VkPipelineLayout m_pipelineLayout;
        VkPipeline m_graphicsPipeline;
        
        // Descriptor set objects
        VkDescriptorSetLayout m_descriptorSetLayout;
        VkDescriptorPool m_descriptorPool;
        std::vector<VkDescriptorSet> m_descriptorSets;
        
        // Uniform buffers
        struct UniformBufferObject {
            glm::mat4 model;
            glm::mat4 view;
            glm::mat4 proj;
        };
        
        std::vector<VkBuffer> m_uniformBuffers;
        std::vector<VmaAllocation> m_uniformBufferAllocations;
        std::vector<void*> m_uniformBufferMapped;
    };

}