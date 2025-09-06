# Motoru statik bir kütüphane olarak tanımla
add_library(AstralEngine STATIC
    Core/Logger.cpp
    Core/ConfigManager.cpp
    Platform/Window.cpp
    Renderer/Renderer.cpp
    Renderer/VMA_Implementation.cpp
    Renderer/Vulkan/VulkanDevice.cpp
    Renderer/Vulkan/VulkanSwapChain.cpp
    Renderer/Vulkan/VulkanPipeline.cpp
    Renderer/Model.cpp
    Renderer/Camera.cpp
)

# Kütüphanenin public arayüzü için include dizinini belirt
target_include_directories(AstralEngine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
