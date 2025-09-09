#include "VulkanPipeline.h"
#include "VulkanDevice.h"
#include "Core/Logger.h"
#include "Renderer/Model.h"
#include "Renderer/Shader.h"
#include <fstream>
#include <stdexcept>

namespace AstralEngine::Vulkan {
VulkanPipeline::VulkanPipeline(VulkanDevice& device, const std::vector<VkDescriptorSetLayout>& setLayouts, VkFormat swapChainImageFormat, VkFormat depthFormat, const Shader& shader)
        : VulkanPipeline(device, setLayouts, swapChainImageFormat, depthFormat, shader, PipelineConfig::createDefault()) {
    }
    
    VulkanPipeline::VulkanPipeline(VulkanDevice& device, const std::vector<VkDescriptorSetLayout>& setLayouts, VkFormat swapChainImageFormat, VkFormat depthFormat, const Shader& shader, const PipelineConfig& config)
        : m_device(device), m_graphicsPipeline(VK_NULL_HANDLE), m_pipelineLayout(VK_NULL_HANDLE), m_pipelineCache(VK_NULL_HANDLE), m_config(config) {
        createPipelineCache();
        createGraphicsPipeline(shader, setLayouts, swapChainImageFormat, depthFormat, config);
    }

    VulkanPipeline::~VulkanPipeline() {
        // Save pipeline cache before destroying it
        savePipelineCache();
        
        if (m_pipelineCache != VK_NULL_HANDLE) {
            vkDestroyPipelineCache(m_device.getDevice(), m_pipelineCache, nullptr);
        }
        
        if (m_graphicsPipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(m_device.getDevice(), m_graphicsPipeline, nullptr);
        }
        
        if (m_pipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(m_device.getDevice(), m_pipelineLayout, nullptr);
        }
    }

    void VulkanPipeline::createPipelineCache() {
        // Try to load existing pipeline cache
        loadPipelineCache();
        
        // If we don't have a cache, create a new one
        if (m_pipelineCache == VK_NULL_HANDLE) {
            VkPipelineCacheCreateInfo cacheInfo{};
            cacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
            
            if (vkCreatePipelineCache(m_device.getDevice(), &cacheInfo, nullptr, &m_pipelineCache) != VK_SUCCESS) {
                throw std::runtime_error("Pipeline cache oluşturulamadı!");
            }
        }
    }

    void VulkanPipeline::loadPipelineCache() {
        // Try to load pipeline cache from file
        std::ifstream file("pipeline_cache.bin", std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
            // No cache file exists, that's fine
            return;
        }
        
        size_t fileSize = (size_t)file.tellg();
        if (fileSize == 0) {
            // Empty cache file
            file.close();
            return;
        }
        
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();
        
        VkPipelineCacheCreateInfo cacheInfo{};
        cacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        cacheInfo.initialDataSize = fileSize;
        cacheInfo.pInitialData = buffer.data();
        
        if (vkCreatePipelineCache(m_device.getDevice(), &cacheInfo, nullptr, &m_pipelineCache) != VK_SUCCESS) {
            // Failed to create cache from data, create a new one
            m_pipelineCache = VK_NULL_HANDLE;
            VkPipelineCacheCreateInfo newCacheInfo{};
            newCacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
            
            if (vkCreatePipelineCache(m_device.getDevice(), &newCacheInfo, nullptr, &m_pipelineCache) != VK_SUCCESS) {
                throw std::runtime_error("Pipeline cache oluşturulamadı!");
            }
        }
    }

    void VulkanPipeline::savePipelineCache() {
        if (m_pipelineCache == VK_NULL_HANDLE) {
            return;
        }
        
        // Get the size of the pipeline cache data
        size_t dataSize = 0;
        vkGetPipelineCacheData(m_device.getDevice(), m_pipelineCache, &dataSize, nullptr);
        
        if (dataSize == 0) {
            return;
        }
        
        // Get the pipeline cache data
        std::vector<char> data(dataSize);
        vkGetPipelineCacheData(m_device.getDevice(), m_pipelineCache, &dataSize, data.data());
        
        // Save to file
        std::ofstream file("pipeline_cache.bin", std::ios::binary);
        if (file.is_open()) {
            file.write(data.data(), dataSize);
            file.close();
        }
    }

void VulkanPipeline::createGraphicsPipeline(const Shader& shader, const std::vector<VkDescriptorSetLayout>& setLayouts, VkFormat swapChainImageFormat, VkFormat depthFormat, const PipelineConfig& config) {
        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = shader.getVertShaderModule();
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = shader.getFragShaderModule();
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        auto bindingDescriptions = AstralEngine::Vertex::getBindingDescriptions();
        auto attributeDescriptions = AstralEngine::Vertex::getAttributeDescriptions();

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = config.topology;
        inputAssembly.primitiveRestartEnable = config.primitiveRestartEnable ? VK_TRUE : VK_FALSE;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = config.rasterization.depthClampEnable ? VK_TRUE : VK_FALSE;
        rasterizer.rasterizerDiscardEnable = config.rasterization.rasterizerDiscardEnable ? VK_TRUE : VK_FALSE;
        rasterizer.polygonMode = config.rasterization.polygonMode;
        rasterizer.lineWidth = config.rasterization.lineWidth;
        rasterizer.cullMode = config.rasterization.cullMode;
        rasterizer.frontFace = config.rasterization.frontFace;
        rasterizer.depthBiasEnable = config.rasterization.depthBiasEnable ? VK_TRUE : VK_FALSE;
        rasterizer.depthBiasConstantFactor = config.rasterization.depthBiasConstantFactor;
        rasterizer.depthBiasClamp = 0.0f;
        rasterizer.depthBiasSlopeFactor = config.rasterization.depthBiasSlopeFactor;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = config.sampleShadingEnable ? VK_TRUE : VK_FALSE;
        multisampling.rasterizationSamples = config.rasterizationSamples;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = config.depthStencil.depthTestEnable ? VK_TRUE : VK_FALSE;
        depthStencil.depthWriteEnable = config.depthStencil.depthWriteEnable ? VK_TRUE : VK_FALSE;
        depthStencil.depthCompareOp = config.depthStencil.depthCompareOp;
        depthStencil.depthBoundsTestEnable = config.depthStencil.depthBoundsTestEnable ? VK_TRUE : VK_FALSE;
        depthStencil.minDepthBounds = config.depthStencil.minDepthBounds;
        depthStencil.maxDepthBounds = config.depthStencil.maxDepthBounds;
        depthStencil.stencilTestEnable = config.depthStencil.stencilTestEnable ? VK_TRUE : VK_FALSE;
        depthStencil.front = config.depthStencil.front;
        depthStencil.back = config.depthStencil.back;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = config.blend.colorWriteMask;
        colorBlendAttachment.blendEnable = config.blend.blendEnable ? VK_TRUE : VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = config.blend.srcColorBlendFactor;
        colorBlendAttachment.dstColorBlendFactor = config.blend.dstColorBlendFactor;
        colorBlendAttachment.colorBlendOp = config.blend.colorBlendOp;
        colorBlendAttachment.srcAlphaBlendFactor = config.blend.srcAlphaBlendFactor;
        colorBlendAttachment.dstAlphaBlendFactor = config.blend.dstAlphaBlendFactor;
        colorBlendAttachment.alphaBlendOp = config.blend.alphaBlendOp;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(config.dynamicStates.size());
        dynamicState.pDynamicStates = config.dynamicStates.data();

        // --- PUSH CONSTANT RANGE EKLEYİN ---
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // Vertex shader'da kullanılacak
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(glm::mat4); // Bir model matrisi göndereceğiz
        // --- PUSH CONSTANT RANGE EKLEME SONU ---

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
        pipelineLayoutInfo.pSetLayouts = setLayouts.data();
        // --- PUSH CONSTANT BİLGİLERİNİ EKLEYİN ---
        pipelineLayoutInfo.pushConstantRangeCount = 1; // Ekle
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange; // Ekle
        // --- PUSH CONSTANT BİLGİLERİ EKLEME SONU ---

        if (vkCreatePipelineLayout(m_device.getDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("Pipeline layout oluşturulamadı!");
        }

        // Enable Dynamic Rendering
        VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo{};
        pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        pipelineRenderingCreateInfo.colorAttachmentCount = 1;
        pipelineRenderingCreateInfo.pColorAttachmentFormats = &swapChainImageFormat;
        pipelineRenderingCreateInfo.depthAttachmentFormat = depthFormat;

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = m_pipelineLayout;
        // Removed renderPass and subpass as they're not needed with Dynamic Rendering
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.pNext = &pipelineRenderingCreateInfo; // Link Dynamic Rendering info

        if (vkCreateGraphicsPipelines(m_device.getDevice(), m_pipelineCache, 1, &pipelineInfo, nullptr, &m_graphicsPipeline) != VK_SUCCESS) {
            throw std::runtime_error("Grafik pipeline'ı oluşturulamadı!");
        }
    }

    void VulkanPipeline::Bind(VkCommandBuffer commandBuffer) {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
    }
}