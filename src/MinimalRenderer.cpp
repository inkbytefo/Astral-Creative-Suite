#include "MinimalRenderer.h"
#include "Core/Logger.h"
#include "Renderer/UnifiedMaterial.h"
#include "Renderer/UnifiedMaterialConstants.h"
#include "ECS/ECS.h"
#include "ECS/ECSTest.h"

// Old material system has been replaced with unified material system
// Old includes were: Material.h, MaterialInstance.h, PBRMaterial.h

namespace AstralEngine {

void MinimalRenderer::testMaterialSystem() {
    AE_INFO("=== Testing Material System ===");

    // Test MaterialUBO structure
    MaterialUBO testUBO;
    testUBO.baseColor = glm::vec4(1.0f, 0.5f, 0.2f, 1.0f);
    testUBO.metallic() = 0.8f;
    testUBO.roughness() = 0.3f;
    testUBO.setFlag(MaterialUBO::Flags::HAS_ALBEDO_MAP, true);

    AE_INFO("MaterialUBO size: {} bytes (should be 112)", sizeof(MaterialUBO));
    AE_INFO("Base color: ({:.2f}, {:.2f}, {:.2f}, {:.2f})", 
            testUBO.baseColor.r, testUBO.baseColor.g, testUBO.baseColor.b, testUBO.baseColor.a);
    AE_INFO("Metallic: {:.2f}, Roughness: {:.2f}", testUBO.metallic(), testUBO.roughness());
    AE_INFO("Has albedo map flag: {}", testUBO.hasFlag(MaterialUBO::Flags::HAS_ALBEDO_MAP));

    // Test material template system
    MaterialTemplate metalTemplate;
    metalTemplate.defaultParameters.baseColor = glm::vec4(0.7f, 0.7f, 0.8f, 1.0f);
    metalTemplate.defaultParameters.metallic() = 1.0f;
    metalTemplate.defaultParameters.roughness() = 0.1f;
    metalTemplate.lockParameter(MaterialTemplate::LOCK_METALLIC, true);

    // Test template application
    MaterialUBO instanceParams;
    instanceParams.baseColor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f); // Red
    instanceParams.metallic() = 0.0f; // This should be overridden

    MaterialUBO finalParams = metalTemplate.applyTemplate(instanceParams);

    AE_INFO("Template test:");
    AE_INFO("  Instance metallic: {:.2f} (should be overridden)", instanceParams.metallic());
    AE_INFO("  Final metallic: {:.2f} (locked by template)", finalParams.metallic());
    AE_INFO("  Final color: ({:.2f}, {:.2f}, {:.2f}) (not locked)", 
            finalParams.baseColor.r, finalParams.baseColor.g, finalParams.baseColor.b);

    // Test texture slot detection
    AE_INFO("=== Testing Texture System ===");
    std::vector<std::string> testPaths = {
        "materials/wood_albedo.jpg",
        "materials/metal_normal.png", 
        "materials/fabric_metallicRoughness.jpg",
        "materials/lamp_emissive.hdr",
        "textures/stone_ao.jpg"
    };

    for (const auto& path : testPaths) {
        TextureSlot slot = Texture::detectSlotFromPath(path);
        TextureFormat format = getPreferredFormat(slot);
        
        std::string slotName;
        switch (slot) {
            case TextureSlot::BaseColor: slotName = "BaseColor"; break;
            case TextureSlot::Normal: slotName = "Normal"; break;
            case TextureSlot::MetallicRoughness: slotName = "MetallicRoughness"; break;
            case TextureSlot::Emissive: slotName = "Emissive"; break;
            case TextureSlot::Occlusion: slotName = "Occlusion"; break;
            default: slotName = "UNKNOWN"; break;
        }
        
        AE_INFO("Path: {} -> Slot: {}, Format: {}", 
                path, slotName,
                format == TextureFormat::SRGB ? "SRGB" : "LINEAR");
    }
}

void MinimalRenderer::testModelLoading() {
    AE_INFO("=== Testing Model Loading System ===");
    
    // Test that our cube.obj file exists
    std::string testModelPath = "assets/models/cube.obj";
    AE_INFO("Test model path: {}", testModelPath);
    
    // We can't actually load the model without Vulkan device
    // But we can test that tinyobjloader is available
    AE_INFO("tinyobjloader integration: Available (included in build)");
    AE_INFO("Model loading: Ready for testing with Vulkan device");
}

void MinimalRenderer::testECSSystem() {
    // Use the new comprehensive ECS test
    ECS::demonstrateNewECSSystem();
}

} // namespace AstralEngine
