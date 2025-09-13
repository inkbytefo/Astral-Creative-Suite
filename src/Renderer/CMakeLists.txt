# Renderer library
set(RENDERER_SOURCES
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
)

# Add refactored Vulkan sources
list(APPEND RENDERER_SOURCES
    # Refactored Vulkan sources
    VulkanR/VulkanInstance.cpp
    VulkanR/VulkanPhysicalDevice.cpp
    VulkanR/VulkanDevice.cpp
    VulkanR/VulkanSwapChain.cpp
    VulkanR/VulkanContext.cpp
    RRenderer.cpp
)

list(APPEND RENDERER_HEADERS
    # Refactored Vulkan headers
    VulkanR/VulkanInstance.h
    VulkanR/VulkanValidation.h
    VulkanR/VulkanPhysicalDevice.h
    VulkanR/VulkanDevice.h
    VulkanR/VulkanSwapChain.h
    VulkanR/VulkanContext.h
    RRenderer.h
)

# Create the AstralRenderer library
add_library(AstralRenderer ${RENDERER_SOURCES} ${RENDERER_HEADERS})

# Define alias for better compatibility
add_library(Astral::Renderer ALIAS AstralRenderer)

# Link dependencies
target_link_libraries(AstralRenderer PUBLIC
    Vulkan::Vulkan
    SDL3::SDL3
    tinyobjloader
    stb_image
    AstralCore
    AstralECS
    imgui
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
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

# Compiler definitions
target_compile_definitions(AstralRenderer PUBLIC GLM_ENABLE_EXPERIMENTAL)
