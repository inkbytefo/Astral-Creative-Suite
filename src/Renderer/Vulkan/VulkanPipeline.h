#pragma once

#include "VulkanContext.h"
#include <vector>

namespace Astral::Vulkan {
    class VulkanPipeline {
    public:
        VulkanPipeline(VulkanDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout descriptorSetLayout, const std::string& vertFilepath, const std::string& fragFilepath);
        ~VulkanPipeline();

        VulkanPipeline(const VulkanPipeline&) = delete;
        VulkanPipeline& operator=(const VulkanPipeline&) = delete;

        void Bind(VkCommandBuffer commandBuffer);
        VkPipelineLayout GetPipelineLayout() const { return m_pipelineLayout; }

    private:
        void createGraphicsPipeline(const std::string& vertFilepath, const std::string& fragFilepath, VkRenderPass renderPass, VkDescriptorSetLayout descriptorSetLayout);
        static std::vector<char> readFile(const std::string& filepath);
        VkShaderModule createShaderModule(const std::vector<char>& code);

        VulkanDevice& m_device;
        VkPipeline m_graphicsPipeline;
        VkPipelineLayout m_pipelineLayout;
    };
}
