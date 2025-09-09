#pragma once

// Minimal renderer for testing our material system
// This doesn't conflict with existing Renderer.cpp

namespace AstralEngine {

class MinimalRenderer {
public:
    static void testMaterialSystem();
    static void testModelLoading();
    static void testECSSystem();
};

} // namespace AstralEngine
