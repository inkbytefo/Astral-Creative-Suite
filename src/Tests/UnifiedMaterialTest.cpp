#include "Renderer/UnifiedMaterial.h"
#include "Renderer/UnifiedMaterialConstants.h"
#include "Core/Logger.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <chrono>

namespace AstralEngine {
namespace Tests {

    void testUnifiedMaterialUBOSize() {
        std::cout << "Testing UnifiedMaterialUBO size and alignment..." << std::endl;
        
        // Verify size is exactly 320 bytes (20 * vec4)
        assert(sizeof(UnifiedMaterialUBO) == 320);
        assert(alignof(UnifiedMaterialUBO) == 16);
        
        // Test default initialization
        UnifiedMaterialUBO ubo;
        assert(ubo.baseColor == glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        assert(ubo.metallic == 0.0f);
        assert(ubo.roughness == 0.5f);
        assert(ubo.emissiveColor == glm::vec3(0.0f));
        
        std::cout << "✓ UnifiedMaterialUBO size and alignment tests passed!" << std::endl;
    }

    void testUnifiedMaterialInstance() {
        std::cout << "Testing UnifiedMaterialInstance..." << std::endl;
        
        // Test default construction
        UnifiedMaterialInstance material;
        assert(material.getBaseColor() == glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        assert(material.getMetallic() == 0.0f);
        assert(material.getRoughness() == 0.5f);
        assert(material.isOpaque());
        assert(!material.isTransparent());
        assert(material.getRenderQueue() == 2000);
        
        // Test property setters
        material.setBaseColor(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
        assert(material.getBaseColor() == glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
        
        material.setMetallic(1.0f);
        assert(material.getMetallic() == 1.0f);
        
        material.setRoughness(0.1f);
        assert(material.getRoughness() == 0.1f);
        
        // Test transparency
        material.setTransparent();
        assert(material.isTransparent());
        assert(!material.isOpaque());
        assert(material.getRenderQueue() == 3000);
        
        // Test alpha test
        material.setAlphaTest(0.5f);
        assert(material.getAlphaMode() == AlphaMode::Mask);
        assert(material.getAlphaCutoff() == 0.5f);
        assert(material.getRenderQueue() == 2500);
        
        // Reset to opaque
        material.setOpaque();
        assert(material.isOpaque());
        
        std::cout << "✓ UnifiedMaterialInstance tests passed!" << std::endl;
    }

    void testAdvancedPBRFeatures() {
        std::cout << "Testing advanced PBR features..." << std::endl;
        
        UnifiedMaterialInstance material;
        
        // Test clearcoat
        material.setClearcoat(1.0f, 0.1f, 1.0f);
        assert(material.hasFeatureFlag(FeatureFlags::UseClearcoat));
        
        // Test anisotropy
        material.setAnisotropy(0.8f, 45.0f);
        assert(material.hasFeatureFlag(FeatureFlags::UseAnisotropy));
        
        // Test sheen
        material.setSheen(0.5f, glm::vec3(1.0f, 0.8f, 0.6f), 0.3f);
        assert(material.hasFeatureFlag(FeatureFlags::UseSheen));
        
        // Test transmission
        material.setTransmission(0.9f, 1.5f, 0.01f);
        assert(material.hasFeatureFlag(FeatureFlags::UseTransmission));
        assert(material.isTransparent()); // Should be transparent with transmission
        
        // Test emissive
        material.setEmissive(glm::vec3(1.0f, 0.5f, 0.0f), 2.0f);
        assert(material.getEmissive() == glm::vec3(2.0f, 1.0f, 0.0f));
        
        std::cout << "✓ Advanced PBR features tests passed!" << std::endl;
    }

    void testMaterialFactory() {
        std::cout << "Testing material factory..." << std::endl;
        
        // Test default material
        auto defaultMat = UnifiedMaterialInstance::createDefault();
        assert(defaultMat.getBaseColor() == glm::vec4(0.8f, 0.8f, 0.8f, 1.0f));
        assert(defaultMat.getMetallic() == 0.0f);
        assert(defaultMat.getRoughness() == 0.5f);
        assert(defaultMat.isOpaque());
        
        // Test metal material
        auto metalMat = UnifiedMaterialInstance::createMetal(glm::vec3(0.9f, 0.7f, 0.3f), 0.1f);
        assert(metalMat.getMetallic() == 1.0f);
        assert(metalMat.getRoughness() == 0.1f);
        assert(metalMat.getBaseColor() == glm::vec4(0.9f, 0.7f, 0.3f, 1.0f));
        
        // Test dielectric material
        auto dielectricMat = UnifiedMaterialInstance::createDielectric(glm::vec3(0.2f, 0.8f, 0.2f), 0.4f);
        assert(dielectricMat.getMetallic() == 0.0f);
        assert(dielectricMat.getRoughness() == 0.4f);
        
        // Test glass material
        auto glassMat = UnifiedMaterialInstance::createGlass(glm::vec3(0.9f, 0.9f, 1.0f), 0.95f);
        assert(glassMat.hasFeatureFlag(FeatureFlags::UseTransmission));
        assert(glassMat.isTransparent());
        
        // Test emissive material
        auto emissiveMat = UnifiedMaterialInstance::createEmissive(glm::vec3(1.0f, 0.3f, 0.1f), 3.0f);
        assert(emissiveMat.getEmissive() == glm::vec3(3.0f, 0.9f, 0.3f));
        
        std::cout << "✓ Material factory tests passed!" << std::endl;
    }

    void testShaderVariants() {
        std::cout << "Testing shader variants..." << std::endl;
        
        UnifiedMaterialInstance material;
        
        // Test basic variant
        std::string basicVariant = material.getShaderVariant();
        uint64_t basicHash = material.getVariantHash();
        
        // Add texture flags and check variant changes
        material.setTextureFlag(TextureFlags::HasBaseColor, true);
        std::string textureVariant = material.getShaderVariant();
        uint64_t textureHash = material.getVariantHash();
        
        assert(textureVariant != basicVariant);
        assert(textureHash != basicHash);
        assert(textureVariant.find("HAS_BASECOLOR_MAP") != std::string::npos);
        
        // Add feature flags
        material.setFeatureFlag(FeatureFlags::UseClearcoat, true);
        std::string featureVariant = material.getShaderVariant();
        uint64_t featureHash = material.getVariantHash();
        
        assert(featureVariant != textureVariant);
        assert(featureHash != textureHash);
        assert(featureVariant.find("USE_CLEARCOAT") != std::string::npos);
        
        // Test hash consistency
        uint64_t hash1 = material.getVariantHash();
        uint64_t hash2 = material.getVariantHash();
        assert(hash1 == hash2);
        
        std::cout << "✓ Shader variant tests passed!" << std::endl;
    }

    void testBackwardCompatibility() {
        std::cout << "Testing backward compatibility..." << std::endl;
        
        // Test type aliases
        MaterialInstance legacyMaterial;  // Should be UnifiedMaterialInstance
        PBRMaterialInstance pbrMaterial; // Should be UnifiedMaterialInstance
        
        // Test factory aliases
        auto defaultFromFactory = MaterialFactory::createDefaultMaterial();
        auto pbrFromFactory = PBRMaterialFactory::createMetalMaterial(glm::vec3(0.5f), 0.2f);
        
        // Test old API methods still work
        legacyMaterial.setBaseColor(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
        legacyMaterial.setMetallic(0.8f);
        legacyMaterial.setRoughness(0.3f);
        
        assert(legacyMaterial.getBaseColor() == glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
        assert(legacyMaterial.getMetallic() == 0.8f);
        assert(legacyMaterial.getRoughness() == 0.3f);
        
        // Test string-based texture access (legacy)
        // material.setTexture("albedo", someTexture);  // Would work with actual texture
        
        std::cout << "✓ Backward compatibility tests passed!" << std::endl;
    }

    void testMaterialTemplate() {
        std::cout << "Testing material template system..." << std::endl;
        
        // Create a template
        MaterialTemplate carPaintTemplate;
        carPaintTemplate.defaultParameters.baseColor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
        carPaintTemplate.defaultParameters.metallic = 0.0f;
        carPaintTemplate.defaultParameters.roughness = 0.6f;
        carPaintTemplate.defaultParameters.getClearcoat() = 1.0f;
        carPaintTemplate.defaultParameters.getClearcoatRoughness() = 0.05f;
        
        // Lock clearcoat parameters
        carPaintTemplate.lockParameter(MaterialTemplate::LOCK_CLEARCOAT, true);
        
        // Create material from template
        UnifiedMaterialInstance carPaint(carPaintTemplate);
        
        // Verify template was applied
        assert(carPaint.getBaseColor() == glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
        assert(carPaint.hasFeatureFlag(FeatureFlags::UseClearcoat));
        
        // Try to modify locked parameter (should be restored by template)
        carPaint.setClearcoat(0.0f);  // Try to disable clearcoat
        // Template should restore it when applied
        
        std::cout << "✓ Material template tests passed!" << std::endl;
    }

    void testRenderQueue() {
        std::cout << "Testing render queue determination..." << std::endl;
        
        // Opaque material
        UnifiedMaterialInstance opaque;
        assert(opaque.getRenderQueue() == 2000);
        assert(opaque.isOpaque());
        
        // Alpha test material
        UnifiedMaterialInstance alphaTest;
        alphaTest.setAlphaTest(0.5f);
        assert(alphaTest.getRenderQueue() == 2500);
        
        // Transparent material
        UnifiedMaterialInstance transparent;
        transparent.setTransparent();
        assert(transparent.getRenderQueue() == 3000);
        assert(transparent.isTransparent());
        
        // Transmission material (should also be transparent)
        UnifiedMaterialInstance glass;
        glass.setTransmission(0.8f);
        assert(glass.isTransparent());
        assert(glass.getRenderQueue() == 3000);
        
        std::cout << "✓ Render queue tests passed!" << std::endl;
    }

    void testMoveSemantics() {
        std::cout << "Testing move semantics..." << std::endl;
        
        // Create original material
        UnifiedMaterialInstance original = UnifiedMaterialInstance::createMetal(glm::vec3(1.0f, 0.0f, 0.0f), 0.1f);
        original.setTextureFlag(TextureFlags::HasBaseColor, true);
        
        uint64_t originalHash = original.getVariantHash();
        
        // Test move constructor
        UnifiedMaterialInstance moved = std::move(original);
        assert(moved.getVariantHash() == originalHash);
        assert(moved.getMetallic() == 1.0f);
        assert(moved.hasTextureFlag(TextureFlags::HasBaseColor));
        
        // Test move assignment
        UnifiedMaterialInstance assigned;
        assigned = std::move(moved);
        assert(assigned.getVariantHash() == originalHash);
        assert(assigned.getMetallic() == 1.0f);
        
        std::cout << "✓ Move semantics tests passed!" << std::endl;
    }

    void performanceTest() {
        std::cout << "Performing performance tests..." << std::endl;
        
        const int NUM_MATERIALS = 1000;
        std::vector<UnifiedMaterialInstance> materials;
        materials.reserve(NUM_MATERIALS);
        
        // Test material creation performance
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < NUM_MATERIALS; ++i) {
            materials.emplace_back(UnifiedMaterialInstance::createDefault());
            materials.back().setBaseColor(glm::vec4(
                static_cast<float>(i) / NUM_MATERIALS,
                0.5f,
                1.0f - static_cast<float>(i) / NUM_MATERIALS,
                1.0f
            ));
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "Created " << NUM_MATERIALS << " materials in " 
                  << duration.count() << " microseconds" << std::endl;
        
        // Test hash generation performance
        start = std::chrono::high_resolution_clock::now();
        
        uint64_t totalHash = 0;
        for (const auto& material : materials) {
            totalHash += material.getVariantHash();
        }
        
        end = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "Generated " << NUM_MATERIALS << " hashes in " 
                  << duration.count() << " microseconds (total: " << totalHash << ")" << std::endl;
        
        std::cout << "✓ Performance tests completed!" << std::endl;
    }

} // namespace Tests

void runUnifiedMaterialTests() {
    std::cout << "=== Running Unified Material System Tests ===" << std::endl;
    
    try {
        Tests::testUnifiedMaterialUBOSize();
        Tests::testUnifiedMaterialInstance();
        Tests::testAdvancedPBRFeatures();
        Tests::testMaterialFactory();
        Tests::testShaderVariants();
        Tests::testBackwardCompatibility();
        Tests::testMaterialTemplate();
        Tests::testRenderQueue();
        Tests::testMoveSemantics();
        Tests::performanceTest();
        
        std::cout << "=== All Unified Material System Tests Passed! ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        throw;
    }
}

} // namespace AstralEngine
