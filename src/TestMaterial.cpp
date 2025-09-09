#include "Core/Logger.h"
#include "MinimalRenderer.h"
#include <iostream>

int main() {
    try {
        AE_INFO("=== AstralEngine Material System Test ===");

        // Test our new material and ECS systems
        AstralEngine::MinimalRenderer::testMaterialSystem();
        AstralEngine::MinimalRenderer::testModelLoading();
        AstralEngine::MinimalRenderer::testECSSystem();

        AE_INFO("=== All Tests Completed Successfully ===");
        return 0;

    } catch (const std::exception& e) {
        AE_ERROR("Test failed: {}", e.what());
        return 1;
    }
}
