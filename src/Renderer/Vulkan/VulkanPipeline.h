#pragma once

#include "VulkanDevice.h"
#include "Renderer/PipelineConfig.h"
#include <vulkan/vulkan.h>
#include <vector>

// Forward declaration
namespace AstralEngine { class Shader; }

namespace AstralEngine::Vulkan {
class VulkanPipeline {
    public:
        // Constructor with default pipeline configuration
        VulkanPipeline(VulkanDevice& device, const std::vector<VkDescriptorSetLayout>& setLayouts, VkFormat swapChainImageFormat, VkFormat depthFormat, const Shader& shader);
        // Constructor with custom pipeline configuration
        VulkanPipeline(VulkanDevice& device, const std::vector<VkDescriptorSetLayout>& setLayouts, VkFormat swapChainImageFormat, VkFormat depthFormat, const Shader& shader, const PipelineConfig& config);
        ~VulkanPipeline();

        VkPipeline getPipeline() const { return m_graphicsPipeline; }
        VkPipelineLayout getPipelineLayout() const { return m_pipelineLayout; }
        VkPipelineCache getPipelineCache() const { return m_pipelineCache; }
        
        void Bind(VkCommandBuffer commandBuffer);

private:
        void createGraphicsPipeline(const Shader& shader, const std::vector<VkDescriptorSetLayout>& setLayouts, VkFormat swapChainImageFormat, VkFormat depthFormat, const PipelineConfig& config);
        void createPipelineCache();
        void loadPipelineCache();
        void savePipelineCache();

        VulkanDevice& m_device;
        VkPipeline m_graphicsPipeline;
        VkPipelineLayout m_pipelineLayout;
        VkPipelineCache m_pipelineCache;
        PipelineConfig m_config;
    };
}