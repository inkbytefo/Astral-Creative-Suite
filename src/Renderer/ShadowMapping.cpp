#include "ShadowMapping.h"
#include "Core/Logger.h"
#include "Renderer/Vulkan/VulkanDevice.h"
#include "Renderer/Vulkan/VulkanBuffer.h"
#include "Renderer/Shader.h"
#include "Renderer/Camera.h"
#include "ECS/Components.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <array>
#include <vector>

namespace AstralEngine {

// ShadowMapManager Implementation
ShadowMapManager::ShadowMapManager(Vulkan::VulkanDevice& device)
    : m_device(device) {
}

ShadowMapManager::~ShadowMapManager() {
    shutdown();
}

bool ShadowMapManager::initialize(const ShadowConfig& config) {
    AE_INFO("Initializing Shadow Map Manager...");
    
    m_config = config;
    
    try {
        createShadowMap();
        createShadowSampler();
        createShadowUBO();
        createRenderPass();
        createFramebuffers();
        createShadowPipeline();
        
        AE_INFO("Shadow Map Manager initialized successfully");
        AE_INFO("- Shadow map size: %ux%u", m_config.shadowMapSize, m_config.shadowMapSize);
        AE_INFO("- Cascade count: %u", m_config.cascadeCount);
        AE_INFO("- Filter mode: %d", static_cast<int>(m_config.filterMode));
        
        return true;
    } catch (const std::exception& e) {
        AE_ERROR("Failed to initialize Shadow Map Manager: {}", e.what());
        AE_ERROR("  - Make sure shadow shaders are compiled and available:");
        AE_ERROR("    - shadow_depth.vert -> shadow_depth.vert.spv");
        AE_ERROR("    - Check shader paths: ./, shaders/, ../shaders/");
        AE_ERROR("  - Shader search locations attempted:");
        AE_ERROR("    1. Current working directory");
        AE_ERROR("    2. ./shaders/ subdirectory");
        AE_ERROR("    3. ../shaders/ parent directory");
        AE_ERROR("  - If shaders are missing, run shader compilation build step");
        
        // Clean up any partial initialization
        shutdown();
        return false;
    }
}

void ShadowMapManager::shutdown() {
    AE_DEBUG("Shadow Map Manager shutdown");
    
    VkDevice device = m_device.getDevice();
    
    // Clean up framebuffers
    for (VkFramebuffer framebuffer : m_framebuffers) {
        if (framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
    }
    m_framebuffers.clear();

    // Destroy cascade image views
    for (auto& view : m_cascadeImageViews) {
        if (view != VK_NULL_HANDLE) {
            vkDestroyImageView(device, view, nullptr);
            view = VK_NULL_HANDLE;
        }
    }
    m_cascadeImageViews.clear();
    
    // Clean up pipeline
    if (m_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }
    
    if (m_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }
    
    // Clean up render pass
    if (m_renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device, m_renderPass, nullptr);
        m_renderPass = VK_NULL_HANDLE;
    }
    
    // Clean up sampler
    if (m_shadowSampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, m_shadowSampler, nullptr);
        m_shadowSampler = VK_NULL_HANDLE;
    }
    
    // Clean up image view
    if (m_shadowMapView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, m_shadowMapView, nullptr);
        m_shadowMapView = VK_NULL_HANDLE;
    }
    
    // Clean up shadow map image
    if (m_shadowMapImage != VK_NULL_HANDLE) {
        vmaDestroyImage(m_device.getAllocator(), m_shadowMapImage, m_shadowMapAllocation);
        m_shadowMapImage = VK_NULL_HANDLE;
        m_shadowMapAllocation = VK_NULL_HANDLE;
    }
    
    // Clean up UBO
    m_shadowUBO.reset();
    
    AE_DEBUG("Shadow Map Manager cleanup completed");
}

void ShadowMapManager::createShadowMap() {
    AE_DEBUG("Creating shadow map texture array");
    
    // Create 2D array texture for cascaded shadow maps
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = m_config.shadowMapSize;
    imageInfo.extent.height = m_config.shadowMapSize;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = m_config.cascadeCount; // One layer per cascade
    imageInfo.format = VK_FORMAT_D32_SFLOAT; // 32-bit depth
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    // Allocation info for VMA
    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    
    VkResult result = vmaCreateImage(m_device.getAllocator(), &imageInfo, &allocInfo, 
                                    &m_shadowMapImage, &m_shadowMapAllocation, nullptr);
    
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shadow map image!");
    }
    
    // Create image view for the array
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_shadowMapImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    viewInfo.format = VK_FORMAT_D32_SFLOAT;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = m_config.cascadeCount;
    
    result = vkCreateImageView(m_device.getDevice(), &viewInfo, nullptr, &m_shadowMapView);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shadow map image view!");
    }
    
    AE_DEBUG("Created shadow map: %ux%u with %u cascades", 
             m_config.shadowMapSize, m_config.shadowMapSize, m_config.cascadeCount);
}

void ShadowMapManager::createShadowSampler() {
    AE_DEBUG("Creating shadow sampler");
    
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE; // Outside shadow map = no shadow
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_TRUE; // Enable shadow comparison
    samplerInfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL; // Standard shadow test
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    
    VkResult result = vkCreateSampler(m_device.getDevice(), &samplerInfo, nullptr, &m_shadowSampler);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shadow sampler!");
    }
    
    AE_DEBUG("Created shadow sampler with comparison enabled");
}

void ShadowMapManager::createShadowUBO() {
    AE_DEBUG("Creating shadow UBO");
    
    // Create uniform buffer for shadow data
    m_shadowUBO = std::make_unique<Vulkan::VulkanBuffer>(
        m_device,
        sizeof(ShadowUBO),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU,
        "ShadowUBO"
    );
    
    // Initialize with default values
    ShadowUBO shadowData;
    shadowData.cascadeDistances = glm::vec4(10.0f, 25.0f, 50.0f, 100.0f); // Default cascade distances
    shadowData.shadowParams = glm::vec4(
        m_config.depthBiasConstant,
        m_config.normalOffsetScale,
        m_config.pcfRadius,
        1.0f // shadow strength
    );
    shadowData.shadowConfig = glm::uvec4(
        m_config.cascadeCount,
        static_cast<uint32_t>(m_config.filterMode),
        m_config.shadowMapSize,
        m_config.visualizeCascades ? 1 : 0
    );
    shadowData.lightDirection = glm::vec3(0.0f, -1.0f, 0.0f); // Default downward light
    
    // Upload initial data
    m_shadowUBO->map();
    m_shadowUBO->copyTo(&shadowData, sizeof(ShadowUBO));
    m_shadowUBO->unmap();
    
    AE_DEBUG("Created shadow UBO with %zu bytes", sizeof(ShadowUBO));
}

void ShadowMapManager::createRenderPass() {
    AE_DEBUG("Creating shadow render pass");
    
    // Shadow render pass only needs depth attachment
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = VK_FORMAT_D32_SFLOAT;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Store for later sampling
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL; // For sampling
    
    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 0;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 0; // No color attachments for shadow pass
    subpass.pColorAttachments = nullptr;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    
    // Subpass dependency to ensure proper synchronization
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependency.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &depthAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;
    
    VkResult result = vkCreateRenderPass(m_device.getDevice(), &renderPassInfo, nullptr, &m_renderPass);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shadow render pass!");
    }
    
    AE_DEBUG("Created shadow render pass for depth-only rendering");
}

void ShadowMapManager::createFramebuffers() {
    AE_DEBUG("Creating shadow framebuffers");
    
    m_framebuffers.resize(m_config.cascadeCount);
    m_cascadeImageViews.resize(m_config.cascadeCount, VK_NULL_HANDLE);
    
    for (uint32_t i = 0; i < m_config.cascadeCount; ++i) {
        // Create individual image view for each cascade layer
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_shadowMapImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // Single layer view
        viewInfo.format = VK_FORMAT_D32_SFLOAT;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = i; // Specific cascade layer
        viewInfo.subresourceRange.layerCount = 1;
        
        VkImageView cascadeView;
        VkResult result = vkCreateImageView(m_device.getDevice(), &viewInfo, nullptr, &cascadeView);
        m_cascadeImageViews[i] = cascadeView;
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create cascade image view!");
        }
        
        // Create framebuffer for this cascade
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &cascadeView;
        framebufferInfo.width = m_config.shadowMapSize;
        framebufferInfo.height = m_config.shadowMapSize;
        framebufferInfo.layers = 1;
        
        result = vkCreateFramebuffer(m_device.getDevice(), &framebufferInfo, nullptr, &m_framebuffers[i]);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create shadow framebuffer!");
        }
        
        AE_DEBUG("Created framebuffer for cascade %u", i);
    }
    
    AE_DEBUG("Created %zu shadow framebuffers", m_framebuffers.size());
}

void ShadowMapManager::beginShadowPass(VkCommandBuffer commandBuffer, uint32_t cascadeIndex) {
    m_currentCascade = cascadeIndex;
    
    if (cascadeIndex >= m_framebuffers.size()) {
        AE_ERROR("Invalid cascade index: %u (max: %zu)", cascadeIndex, m_framebuffers.size() - 1);
        return;
    }
    
    // Begin render pass for this cascade
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_renderPass;
    renderPassInfo.framebuffer = m_framebuffers[cascadeIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = {m_config.shadowMapSize, m_config.shadowMapSize};
    
    // Clear depth to 1.0 (far plane)
    VkClearValue clearValue{};
    clearValue.depthStencil = {1.0f, 0};
    
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearValue;
    
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    
    // Set viewport for shadow map
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_config.shadowMapSize);
    viewport.height = static_cast<float>(m_config.shadowMapSize);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = {m_config.shadowMapSize, m_config.shadowMapSize};
    
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    
    // Set depth bias for shadow acne prevention
    vkCmdSetDepthBias(commandBuffer, 
                     m_config.depthBiasConstant, 
                     0.0f, // clamp
                     m_config.depthBiasSlope);
}

void ShadowMapManager::endShadowPass(VkCommandBuffer commandBuffer) {
    vkCmdEndRenderPass(commandBuffer);
    
    // Transition shadow map to shader read-only layout
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_shadowMapImage;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = m_currentCascade;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    
    vkCmdPipelineBarrier(commandBuffer,
                        VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                        0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void ShadowMapManager::updateShadowUBO(const DirectionalLightShadow& shadowData) {
    if (!m_shadowUBO) {
        AE_ERROR("Shadow UBO not initialized!");
        return;
    }
    
    ShadowUBO uboData;
    
    // Update light space matrices from cascades
    for (uint32_t i = 0; i < shadowData.activeCascadeCount && i < 4; ++i) {
        uboData.lightSpaceMatrices[i] = shadowData.cascades[i].viewProjMatrix;
    }
    
    // Update cascade distances
    for (uint32_t i = 0; i < shadowData.activeCascadeCount && i < 4; ++i) {
        uboData.cascadeDistances[i] = shadowData.cascades[i].splitDistance;
    }
    
    // Update shadow parameters
    uboData.shadowParams = glm::vec4(
        m_config.depthBiasConstant,
        m_config.normalOffsetScale,
        m_config.pcfRadius,
        shadowData.shadowStrength
    );
    
    // Update shadow config
    uboData.shadowConfig = glm::uvec4(
        shadowData.activeCascadeCount,
        static_cast<uint32_t>(m_config.filterMode),
        m_config.shadowMapSize,
        m_config.visualizeCascades ? 1 : 0
    );
    
    // Update light direction
    uboData.lightDirection = shadowData.lightDirection;
    
    // Upload to GPU
    m_shadowUBO->map();
    m_shadowUBO->copyTo(&uboData, sizeof(ShadowUBO));
    m_shadowUBO->unmap();
}

void ShadowMapManager::calculateDirectionalLightCascades(
    const ECS::Light& directionalLight,
    const ECS::Camera& camera,
    const ECS::Transform& cameraTransform,
    DirectionalLightShadow& shadowData
) {
    shadowData.activeCascadeCount = std::min<uint32_t>(m_config.cascadeCount, 4u);
    shadowData.lightDirection = directionalLight.direction;
    shadowData.shadowStrength = 1.0f;
    
    // Use camera's actual projection parameters for accurate frustum calculation
    float nearPlane = camera.nearPlane;
    float farPlane = std::min<float>(camera.farPlane, m_config.maxShadowDistance);
    
    // Ensure valid depth range
    if (nearPlane >= farPlane) {
        AE_WARN("Invalid camera depth range: near={}, far={}, using defaults", nearPlane, farPlane);
        nearPlane = 0.1f;
        farPlane = m_config.maxShadowDistance;
    }
    
    // Calculate optimal cascade split distances
    auto splits = ShadowUtils::computeCascadeSplits(
        nearPlane, farPlane, 
        shadowData.activeCascadeCount, m_config.cascadeSplitLambda
    );
    
    // Configure tight-fitting shadow settings
    CascadeSettings settings;
    settings.splitLambda = m_config.cascadeSplitLambda;
    settings.zPadding = 20.0f;  // Extra depth padding for safety
    settings.tightZ = false;    // Use stable Z for less shimmer
    settings.enableTexelSnapping = true;  // Reduce shadow swimming
    settings.aabbEpsilon = 1.0f;  // Small expansion to avoid clipping
    settings.worldUp = glm::vec3(0.0f, 0.0f, 1.0f);  // Z-up coordinate system
    
    AE_DEBUG("Computing tight-fitting shadow cascades:");
    AE_DEBUG("- Cascade count: {}", shadowData.activeCascadeCount);
    AE_DEBUG("- Shadow map size: {}x{}", m_config.shadowMapSize, m_config.shadowMapSize);
    AE_DEBUG("- Depth range: [{:.2f}, {:.2f}]", nearPlane, farPlane);
    AE_DEBUG("- Split lambda: {:.2f}", settings.splitLambda);
    
    // Calculate tight-fitting light matrices for each cascade
    for (uint32_t i = 0; i < shadowData.activeCascadeCount; ++i) {
        float splitNear = splits[i];
        float splitFar = splits[i + 1];
        
        shadowData.cascades[i].splitDistance = splitFar;
        
        // Create temporary old Camera from ECS components for ShadowUtils compatibility
        Camera tempCamera;
        tempCamera.SetPosition(cameraTransform.position);
        
        // Calculate target from transform rotation (forward direction)
        glm::vec3 forward = glm::normalize(glm::vec3(
            cos(cameraTransform.rotation.y) * cos(cameraTransform.rotation.x),
            sin(cameraTransform.rotation.x),
            sin(cameraTransform.rotation.y) * cos(cameraTransform.rotation.x)
        ));
        tempCamera.SetTarget(cameraTransform.position + forward);
        tempCamera.SetPerspective(camera.fov, camera.aspectRatio, camera.nearPlane, camera.farPlane);
        
        // Use UPGRADED tight-fitting algorithm
        shadowData.cascades[i].viewProjMatrix = ShadowUtils::calculateDirectionalLightMatrix(
            tempCamera,                  // Camera for frustum calculation
            shadowData.lightDirection,   // Light direction
            splitNear,                   // Near plane for this cascade
            splitFar,                    // Far plane for this cascade
            m_config.shadowMapSize,      // Shadow map resolution for texel snapping
            settings                     // Tight-fitting configuration
        );
        
        // For debugging: also store separate view and projection matrices
        // Note: These are approximations since the actual matrices are computed internally
        auto frustumCorners = ShadowUtils::getFrustumCornersWorldSpace(tempCamera, splitNear, splitFar);
        shadowData.cascades[i].viewMatrix = ShadowUtils::buildDirectionalLightView(
            shadowData.lightDirection, frustumCorners, settings.worldUp
        );
        
        // Compute tight projection bounds for debugging
        glm::vec3 minLS, maxLS;
        ShadowUtils::computeLightSpaceAABB(frustumCorners, shadowData.cascades[i].viewMatrix, minLS, maxLS);
        shadowData.cascades[i].projMatrix = ShadowUtils::buildTightOrthoFromAABB(
            minLS - glm::vec3(settings.aabbEpsilon),
            maxLS + glm::vec3(settings.aabbEpsilon),
            settings.zPadding,
            settings.tightZ
        );
        
        AE_DEBUG("Cascade %zu: range=[%.2f, %.2f], bounds=(%.1f,%.1f) to (%.1f,%.1f)",
                 i, splitNear, splitFar, minLS.x, minLS.y, maxLS.x, maxLS.y);
    }
    
    AE_DEBUG("Calculated %u tight-fitting shadow cascades", shadowData.activeCascadeCount);
}

// Compatibility implementation for deprecated getFrustumCornersWorldSpace
std::vector<glm::vec3> ShadowMapManager::getFrustumCornersWorldSpace(
    const glm::mat4& proj, 
    const glm::mat4& view,
    float nearPlane,
    float farPlane
) const {
    AE_WARN("Using deprecated ShadowMapManager::getFrustumCornersWorldSpace - consider using ShadowUtils version");
    
    // This is a basic fallback implementation - the new Camera-based version is preferred
    std::vector<glm::vec3> corners;
    corners.reserve(8);
    
    // NDC cube corners (normalized device coordinates)
    std::array<glm::vec4, 8> ndcCorners = {{
        {-1, -1, -1, 1}, {1, -1, -1, 1}, {-1, 1, -1, 1}, {1, 1, -1, 1},  // Near plane
        {-1, -1,  1, 1}, {1, -1,  1, 1}, {-1, 1,  1, 1}, {1, 1,  1, 1}   // Far plane
    }};
    
    glm::mat4 invViewProj = glm::inverse(proj * view);
    
    for (const auto& ndcCorner : ndcCorners) {
        glm::vec4 worldCorner = invViewProj * ndcCorner;
        worldCorner /= worldCorner.w;  // Perspective divide
        corners.push_back(glm::vec3(worldCorner));
    }
    
    return corners;
}

void ShadowMapManager::setConfig(const ShadowConfig& config) {
    m_config = config;
    AE_DEBUG("Shadow config updated");
}

void ShadowMapManager::printDebugInfo() const {
    AE_INFO("=== Shadow Map Manager Debug Info ===");
    AE_INFO("Shadow map size: %ux%u", m_config.shadowMapSize, m_config.shadowMapSize);
    AE_INFO("Cascade count: %u", m_config.cascadeCount);
    AE_INFO("Max shadow distance: %.1f", m_config.maxShadowDistance);
    AE_INFO("====================================");
}

void ShadowMapManager::createShadowPipeline() {
    AE_DEBUG("Creating shadow pipeline");
    
    // Create pipeline layout with push constants for light view-proj and world matrix
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(glm::mat4) * 2; // Light view-proj + model matrix
    
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0; // No descriptor sets needed for simple depth-only rendering
    pipelineLayoutInfo.pSetLayouts = nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    
    VkResult result = vkCreatePipelineLayout(m_device.getDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shadow pipeline layout!");
    }
    
    // Load depth-only shaders - try multiple paths
    AE_DEBUG("Loading shadow depth shader: shadow_depth.vert");
    Shader depthShader(m_device, "shadow_depth.vert"); // Depth-only vertex shader
    
    // Vertex input description (same as main renderer)
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(float) * 11; // position(3) + color(3) + normal(3) + texcoord(2)
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};
    // Position
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = 0;
    // Color (not used but needed for vertex format compatibility)
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = sizeof(float) * 3;
    // Normal (not used but needed for vertex format compatibility)
    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[2].offset = sizeof(float) * 6;
    // TexCoord (not used but needed for vertex format compatibility)
    attributeDescriptions[3].binding = 0;
    attributeDescriptions[3].location = 3;
    attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[3].offset = sizeof(float) * 9;
    
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
    
    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
    
    // Viewport state
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = nullptr; // Dynamic
    viewportState.scissorCount = 1;
    viewportState.pScissors = nullptr; // Dynamic
    
    // Rasterization
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT; // Front-face culling for shadow maps
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_TRUE; // Enable depth bias for shadow mapping
    rasterizer.depthBiasConstantFactor = m_config.depthBiasConstant;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = m_config.depthBiasSlope;
    
    // Multisampling (disabled)
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    
    // Depth/stencil testing
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;
    
    // No color blending (depth-only pass)
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 0; // No color attachments
    colorBlending.pAttachments = nullptr;
    
    // Dynamic state
    std::array<VkDynamicState, 2> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();
    
    // Pipeline shader stages (vertex only for depth-only rendering)
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = depthShader.getVertShaderModule();
    vertShaderStageInfo.pName = "main";
    
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages = { vertShaderStageInfo };
    
    // Add fragment stage if available (for compatibility)
    if (depthShader.hasFragmentShader() && depthShader.getFragShaderModule() != VK_NULL_HANDLE) {
        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = depthShader.getFragShaderModule();
        fragShaderStageInfo.pName = "main";
        shaderStages.push_back(fragShaderStageInfo);
    }
    
    // Create graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = m_renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;
    
    result = vkCreateGraphicsPipelines(m_device.getDevice(), VK_NULL_HANDLE, 1, 
                                      &pipelineInfo, nullptr, &m_pipeline);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shadow graphics pipeline!");
    }
    
    AE_DEBUG("Created shadow pipeline for depth-only rendering");
}

void ShadowMapManager::bindShadowPipeline(VkCommandBuffer commandBuffer) {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
}

VkPipelineLayout ShadowMapManager::getShadowPipelineLayout() const {
    return m_pipelineLayout;
}

// ============================================================================
// TIGHT-FITTING CASCADED SHADOW MAPS IMPLEMENTATION
// ============================================================================
//
// This implementation provides AAA-quality cascaded shadow mapping using
// tight-fitting orthographic projections. Key improvements over fixed bounds:
//
// 1. FRUSTUM-FIT PROJECTION: Each cascade's shadow map projection is calculated
//    to exactly fit the camera's view frustum slice, maximizing resolution usage.
//
// 2. PRACTICAL SPLIT SCHEME: Cascade distances use a blend of logarithmic and
//    uniform distribution for optimal shadow detail distribution.
//
// 3. TEXEL SNAPPING: Shadow matrices are snapped to texel boundaries to reduce
//    swimming/shimmering artifacts during camera movement.
//
// 4. STABLE Z BOUNDS: Z-range uses stable depth bounds rather than tight-fitting
//    to minimize light matrix changes that cause temporal artifacts.
//
// COORDINATE SYSTEM: Right-handed with Z-up world coordinates (GLM/OpenGL style)
// DEPTH CONVENTION: Standard OpenGL depth [-1, +1] normalized device coordinates
//
// ============================================================================

namespace ShadowUtils {

// Improved cascade split calculation using practical split scheme
std::vector<float> computeCascadeSplits(float nearPlane, float farPlane, uint32_t cascadeCount, float lambda) {
    std::vector<float> splits(cascadeCount + 1);
    
    splits[0] = nearPlane;
    splits[cascadeCount] = farPlane;
    
    for (uint32_t i = 1; i < cascadeCount; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(cascadeCount);
        
        // Logarithmic split for better shadow detail distribution
        float logSplit = nearPlane * std::pow(farPlane / nearPlane, t);
        
        // Uniform split for even cascade coverage
        float uniformSplit = nearPlane + t * (farPlane - nearPlane);
        
        // Practical split: blend between logarithmic and uniform
        splits[i] = lambda * logSplit + (1.0f - lambda) * uniformSplit;
    }
    
    return splits;
}

// Calculate the 8 corners of a camera frustum slice in world space
std::array<glm::vec3, 8> getFrustumCornersWorldSpace(
    const Camera& camera,
    float splitNear,
    float splitFar
) {
    // Get camera parameters
    const glm::vec3 pos = camera.GetPosition();
    const glm::vec3 forward = camera.GetForward();
    const glm::vec3 right = camera.GetRight();
    const glm::vec3 up = camera.GetTrueUp();
    
    const float tanHalfFOV = std::tan(0.5f * camera.GetFOV());
    const float aspect = camera.GetAspect();
    
    // Calculate frustum dimensions at near and far planes
    const float nearH = tanHalfFOV * splitNear;
    const float nearW = nearH * aspect;
    const float farH = tanHalfFOV * splitFar;
    const float farW = farH * aspect;
    
    // Near and far plane centers
    const glm::vec3 nearCenter = pos + forward * splitNear;
    const glm::vec3 farCenter = pos + forward * splitFar;
    
    // 8 frustum corners in world space
    // Near plane: Top-Left, Top-Right, Bottom-Left, Bottom-Right
    // Far plane: Top-Left, Top-Right, Bottom-Left, Bottom-Right
    std::array<glm::vec3, 8> corners = {{
        // Near plane corners
        nearCenter + up * nearH - right * nearW,  // Near Top-Left
        nearCenter + up * nearH + right * nearW,  // Near Top-Right
        nearCenter - up * nearH - right * nearW,  // Near Bottom-Left
        nearCenter - up * nearH + right * nearW,  // Near Bottom-Right
        
        // Far plane corners
        farCenter + up * farH - right * farW,     // Far Top-Left
        farCenter + up * farH + right * farW,     // Far Top-Right
        farCenter - up * farH - right * farW,     // Far Bottom-Left
        farCenter - up * farH + right * farW      // Far Bottom-Right
    }};
    
    return corners;
}

// Build directional light view matrix centered on frustum corners
glm::mat4 buildDirectionalLightView(
    const glm::vec3& lightDirection,
    const std::array<glm::vec3, 8>& frustumCorners,
    const glm::vec3& worldUp
) {
    // Calculate frustum center
    glm::vec3 center(0.0f);
    for (const auto& corner : frustumCorners) {
        center += corner;
    }
    center /= static_cast<float>(frustumCorners.size());
    
    // Calculate bounding radius for consistent light position
    float maxRadius = 0.0f;
    for (const auto& corner : frustumCorners) {
        float distance = glm::length(corner - center);
        maxRadius = std::max<float>(maxRadius, distance);
    }
    
    // Quantize radius to reduce shadow shimmer during movement
    const float quantization = 16.0f;
    maxRadius = std::ceil(maxRadius * quantization) / quantization;
    
    // Normalize light direction
    const glm::vec3 lightDir = glm::normalize(lightDirection);
    
    // Choose up vector, avoiding degeneracy when light is vertical
    glm::vec3 up = worldUp;
    if (std::abs(glm::dot(up, lightDir)) > 0.95f) {
        up = glm::vec3(0.0f, 1.0f, 0.0f);  // Fallback to Y-up
        if (std::abs(glm::dot(up, lightDir)) > 0.95f) {
            up = glm::vec3(1.0f, 0.0f, 0.0f);  // Final fallback to X-up
        }
    }
    
    // Position light camera behind the frustum center
    const float zPadding = 50.0f;  // Extra depth to ensure all geometry is captured
    const glm::vec3 lightPos = center - lightDir * (maxRadius + zPadding);
    
    return glm::lookAt(lightPos, center, up);
}

// Compute tight AABB of frustum corners in light space
void computeLightSpaceAABB(
    const std::array<glm::vec3, 8>& frustumCorners,
    const glm::mat4& lightView,
    glm::vec3& outMinLS,
    glm::vec3& outMaxLS
) {
    // Initialize bounds with first corner
    glm::vec4 firstCornerLS = lightView * glm::vec4(frustumCorners[0], 1.0f);
    outMinLS = outMaxLS = glm::vec3(firstCornerLS);
    
    // Expand bounds to include all corners
    for (size_t i = 1; i < frustumCorners.size(); ++i) {
        glm::vec4 cornerLS = lightView * glm::vec4(frustumCorners[i], 1.0f);
        glm::vec3 cornerLS3 = glm::vec3(cornerLS);
        
        outMinLS = glm::min(outMinLS, cornerLS3);
        outMaxLS = glm::max(outMaxLS, cornerLS3);
    }
}

// Create tight orthographic projection from light-space AABB
glm::mat4 buildTightOrthoFromAABB(
    const glm::vec3& minLS,
    const glm::vec3& maxLS,
    float zPadding,
    bool tightZ
) {
    // Tight X and Y bounds
    const float left = minLS.x;
    const float right = maxLS.x;
    const float bottom = minLS.y;
    const float top = maxLS.y;
    
    // Z bounds handling
    float near, far;
    if (tightZ) {
        // Tight Z bounds: use actual frustum depth range
        near = -maxLS.z - zPadding;
        far = -minLS.z + zPadding;
    } else {
        // Stable Z bounds: fixed depth range to reduce shimmer
        const float depthRange = maxLS.z - minLS.z;
        near = zPadding;
        far = depthRange + 2.0f * zPadding;
    }
    
    // Ensure valid depth range
    if (near >= far) {
        near = 0.1f;
        far = near + std::max<float>(1.0f, far - near);
    }
    
    return glm::ortho(left, right, bottom, top, near, far);
}

// Apply texel snapping to reduce shadow shimmering
void applyTexelSnapping(
    glm::mat4& lightView,
    glm::mat4& lightProj,
    uint32_t shadowMapResolution
) {
    // Extract orthographic projection bounds
    const float left = -(lightProj[3][0] / lightProj[0][0]) / (1.0f / lightProj[0][0]);
    const float right = left + 2.0f / lightProj[0][0];
    const float bottom = -(lightProj[3][1] / lightProj[1][1]) / (1.0f / lightProj[1][1]);
    const float top = bottom + 2.0f / lightProj[1][1];
    
    // Calculate texel size in light space
    const float texelSizeX = (right - left) / static_cast<float>(shadowMapResolution);
    const float texelSizeY = (top - bottom) / static_cast<float>(shadowMapResolution);
    
    // Extract light space center
    glm::vec3 centerWS = glm::vec3(0.0f);
    glm::vec4 centerLS = lightView * glm::vec4(centerWS, 1.0f);
    
    // Snap center to texel grid
    centerLS.x = std::floor(centerLS.x / texelSizeX) * texelSizeX;
    centerLS.y = std::floor(centerLS.y / texelSizeY) * texelSizeY;
    
    // Apply snapping offset to light view matrix
    glm::mat4 snapOffset = glm::mat4(1.0f);
    snapOffset[3][0] = centerLS.x;
    snapOffset[3][1] = centerLS.y;
    
    lightView = snapOffset * lightView;
}

// UPGRADED: Calculate light space matrix with tight-fitting projection
glm::mat4 calculateDirectionalLightMatrix(
    const Camera& camera,
    const glm::vec3& lightDirection,
    float splitNear,
    float splitFar,
    uint32_t shadowMapResolution,
    const CascadeSettings& settings
) {
    // Step 1: Get frustum corners for this cascade
    auto frustumCorners = getFrustumCornersWorldSpace(camera, splitNear, splitFar);
    
    // Step 2: Build light view matrix centered on frustum
    glm::mat4 lightView = buildDirectionalLightView(
        lightDirection, frustumCorners, settings.worldUp
    );
    
    // Step 3: Compute tight AABB in light space
    glm::vec3 minLS, maxLS;
    computeLightSpaceAABB(frustumCorners, lightView, minLS, maxLS);
    
    // Expand AABB slightly to avoid clipping
    const glm::vec3 epsilon(settings.aabbEpsilon);
    minLS -= epsilon;
    maxLS += epsilon;
    
    // Step 4: Build tight orthographic projection
    glm::mat4 lightProj = buildTightOrthoFromAABB(
        minLS, maxLS, settings.zPadding, settings.tightZ
    );
    
    // Step 5: Apply texel snapping if enabled
    if (settings.enableTexelSnapping) {
        applyTexelSnapping(lightView, lightProj, shadowMapResolution);
    }
    
    return lightProj * lightView;
}

// LEGACY: Old calculateDirectionalLightMatrix for compatibility
glm::mat4 calculateDirectionalLightMatrix(
    const glm::vec3& lightDirection,
    const std::vector<glm::vec3>& sceneCorners,
    float zMult
) {
    AE_WARN("Using deprecated calculateDirectionalLightMatrix - consider upgrading to Camera-based version");
    
    // Simple fallback implementation
    glm::vec3 center(0.0f);
    for (const auto& corner : sceneCorners) {
        center += corner;
    }
    center /= static_cast<float>(sceneCorners.size());
    
    glm::mat4 lightView = glm::lookAt(
        center - lightDirection * (zMult * 5.0f), 
        center, 
        glm::vec3(0, 0, 1)
    );
    
    // Use larger bounds than before, but still fixed
    glm::mat4 lightProj = glm::ortho(-100.0f, 100.0f, -100.0f, 100.0f, 0.1f, zMult * 10.0f);
    
    return lightProj * lightView;
}

glm::vec4 calculateBoundingSphere(const std::vector<glm::vec3>& points) {
    if (points.empty()) return glm::vec4(0.0f);
    
    // Simple centroid-based bounding sphere
    glm::vec3 center(0.0f);
    for (const auto& point : points) {
        center += point;
    }
    center /= points.size();
    
    float maxRadius = 0.0f;
    for (const auto& point : points) {
        float distance = glm::length(point - center);
        maxRadius = std::max<float>(maxRadius, distance);
    }
    
    return glm::vec4(center, maxRadius);
}

glm::mat4 stabilizeProjectionMatrix(const glm::mat4& projMatrix, uint32_t shadowMapSize) {
    // Stub - TODO: Implement shadow stabilization
    return projMatrix;
}

std::vector<glm::vec3> getCascadeColors(uint32_t cascadeCount) {
    std::vector<glm::vec3> colors;
    colors.reserve(cascadeCount);
    
    for (uint32_t i = 0; i < cascadeCount; ++i) {
        float t = static_cast<float>(i) / std::max(1u, cascadeCount - 1);
        // Generate distinct colors for each cascade
        colors.emplace_back(
            0.5f + 0.5f * std::sin(t * 6.28f),
            0.5f + 0.5f * std::sin(t * 6.28f + 2.09f),
            0.5f + 0.5f * std::sin(t * 6.28f + 4.18f)
        );
    }
    
    return colors;
}

void debugDrawCascadeFrustum(const ShadowCascade& cascade, const glm::vec3& color) {
    // Stub - TODO: Implement debug visualization
}

// Legacy compatibility wrapper
std::vector<float> calculateCSMSplits(float nearPlane, float farPlane, uint32_t cascadeCount, float lambda) {
    AE_WARN("Using deprecated calculateCSMSplits - use computeCascadeSplits instead");
    return computeCascadeSplits(nearPlane, farPlane, cascadeCount, lambda);
}

} // namespace ShadowUtils

// ShadowRenderer implementation
ShadowRenderer::ShadowRenderer(Vulkan::VulkanDevice& device, ShadowMapManager& shadowManager)
    : m_device(device), m_shadowManager(shadowManager) {
}

void ShadowRenderer::renderShadowMaps(
    VkCommandBuffer commandBuffer,
    const std::vector<LightShadowData>& lightShadows,
    const std::vector<class RenderObject>& objects
) {
    // Stub - TODO: Implement shadow rendering
    AE_DEBUG("Rendering shadow maps for %zu lights", lightShadows.size());
}

void ShadowRenderer::bindShadowResources(
    VkCommandBuffer commandBuffer,
    VkPipelineLayout pipelineLayout,
    uint32_t descriptorSetIndex
) {
    // Stub - TODO: Implement shadow resource binding
}

void ShadowRenderer::renderDirectionalShadow(
    VkCommandBuffer commandBuffer,
    const DirectionalLightShadow& shadowData,
    const std::vector<class RenderObject>& objects
) {
    // Stub - TODO: Implement directional shadow rendering
}

} // namespace AstralEngine
