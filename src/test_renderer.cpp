#include "Renderer/VulkanR/Vulkan.h"
#include "ECS/RenderComponents.h"
#include "Core/Logger.h"
#include <iostream>

int main() {
    // Initialize logger
    AstralEngine::Logger::Init();
    
    std::cout << "Renderer Component Test" << std::endl;
    
    // Test that we can include and use the renderer components
    std::cout << "Renderer components included successfully!" << std::endl;
    
    // Cleanup logger
    AstralEngine::Logger::Shutdown();
    
    return 0;
}