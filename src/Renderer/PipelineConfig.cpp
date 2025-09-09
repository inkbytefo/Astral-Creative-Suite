#include "PipelineConfig.h"
#include "UnifiedMaterialConstants.h"

namespace AstralEngine {

PipelineConfig getPipelineConfigFromMaterialFlags(uint32_t materialFlags) {
    PipelineConfig config = PipelineConfig::createDefault();
    
    // Check material flags to determine appropriate pipeline configuration
    // Note: The UnifiedMaterialUBO uses different flag organization
    // For now, keep this as a placeholder function
    // TODO: Update to work with the new UnifiedMaterialUBO flag system
    
    return config;
}

PipelineConfig getPipelineConfigFromUnifiedMaterial(const UnifiedMaterialUBO& ubo) {
    PipelineConfig config = PipelineConfig::createDefault();
    
    // Check alpha mode for transparency
    AlphaMode alphaMode = getAlphaMode(ubo);
    if (alphaMode == AlphaMode::Blend) {
        config = PipelineConfig::createTransparent();
    }
    
    // Check for double-sided materials
    if (ubo.hasRenderingFlag(RenderingFlags::DoubleSided)) {
        config.rasterization.cullMode = VK_CULL_MODE_NONE;
    }
    
    // Alpha test requires specific depth stencil settings
    if (alphaMode == AlphaMode::Mask) {
        config.depthStencil.depthTestEnable = true;
        config.depthStencil.depthWriteEnable = true;
        // Alpha testing is handled in the fragment shader
    }
    
    return config;
}

} // namespace AstralEngine
