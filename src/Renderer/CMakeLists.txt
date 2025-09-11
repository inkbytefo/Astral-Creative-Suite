# Renderer library
set(RENDERER_SOURCES
    Renderer.cpp
    RenderSystem.cpp
    2D/CanvasRenderer.cpp
    3D/SceneRenderer.cpp
    Camera.cpp
    MaterialShaderManager.cpp
    Model.cpp
    PipelineConfig.cpp
    Shader.cpp
    ShaderHotReload.cpp
    ShadowMapping.cpp
    Texture.cpp
    UnifiedMaterial.cpp
    UnifiedMaterialConstants.cpp
    VMA_Implementation.cpp
)

set(RENDERER_HEADERS
    Renderer.h
    RenderSystem.h
    2D/CanvasRenderer.h
    3D/SceneRenderer.h
    Camera.h
    MaterialShaderManager.h
    Model.h
    PipelineConfig.h
    Shader.h
    ShaderHotReload.h
    ShadowMapping.h
    Texture.h
    UnifiedMaterial.h
    UnifiedMaterialConstants.h
    VMA_Implementation.cpp
)

# Add Vulkan sources
list(APPEND RENDERER_SOURCES
    Vulkan/DescriptorSetManager.cpp
    Vulkan/VulkanBuffer.cpp
    Vulkan/VulkanDevice.cpp
    Vulkan/VulkanPipeline.cpp
    Vulkan/VulkanSwapChain.cpp
    Vulkan/VulkanUtils.cpp
)

list(APPEND RENDERER_HEADERS
    Vulkan/DescriptorSetManager.h
    Vulkan/VulkanBuffer.h
    Vulkan/VulkanContext.h
    Vulkan/VulkanDevice.h
    Vulkan/VulkanPipeline.h
    Vulkan/VulkanSwapChain.h
    Vulkan/VulkanUtils.h
)

# Create the AstralRenderer library
add_library(AstralRenderer ${RENDERER_SOURCES} ${RENDERER_HEADERS})

# Link dependencies
target_link_libraries(AstralRenderer PUBLIC
    Vulkan::Vulkan
    SDL3::SDL3
    tinyobjloader
    stb_image
)

# Conditionally link fmt if available
if(TARGET fmt::fmt)
    target_link_libraries(AstralRenderer PUBLIC fmt::fmt)
endif()

# Include directories
target_include_directories(AstralRenderer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${PROJECT_SOURCE_DIR}/external/glm
    ${PROJECT_SOURCE_DIR}/external/tinyobjloader
    ${PROJECT_SOURCE_DIR}/external/vma/include
)

# Compiler definitions
target_compile_definitions(AstralRenderer PUBLIC GLM_ENABLE_EXPERIMENTAL)