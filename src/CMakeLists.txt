# Motoru statik bir kütüphane olarak tanımla
add_library(AstralEngine STATIC
    # Core modules
    Core/Logger.cpp
    Core/ConfigManager.cpp
    Core/AssetManager.cpp
    Core/ThreadPool.cpp
    Core/PerformanceMonitor.cpp
    Core/EngineConfig.cpp
    Core/AssetDependency.cpp
    
    # Platform abstraction
    Platform/Window.cpp
    
    # Renderer modules
    Renderer/Renderer.cpp
    Renderer/VMA_Implementation.cpp
    Renderer/Vulkan/VulkanDevice.cpp
    Renderer/Vulkan/VulkanSwapChain.cpp
    Renderer/Vulkan/VulkanPipeline.cpp
    Renderer/Vulkan/VulkanBuffer.cpp
    Renderer/Vulkan/VulkanUtils.cpp
    Renderer/Vulkan/DescriptorSetManager.cpp
    Renderer/Model.cpp
    Renderer/Camera.cpp
    Renderer/Texture.cpp
    Renderer/UnifiedMaterial.cpp
    Renderer/UnifiedMaterialConstants.cpp
    Renderer/Shader.cpp
    
    # ECS modules (modern archetype-based system)
    ECS/Components.cpp
    ECS/Scene.cpp
    ECS/ArchetypeECS.cpp
    # ECS/ECSTest.cpp  # Test file - not part of main library
    
    # Memory management
    Core/MemoryManager.cpp
    
    # Advanced rendering
    Renderer/ShadowMapping.cpp
    Renderer/MaterialShaderManager.cpp
    
    # Legacy Entity system removed - use ArchetypeECS instead
)

# Kütüphanenin public arayüzü için include dizinini belirt
target_include_directories(AstralEngine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})