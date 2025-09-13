#pragma once

#include <memory>
#include <vector>
#include <imgui.h>

// Forward Declarations
class Window;
namespace AstralEngine {
    namespace ECS {
        class Scene;
    }
    namespace VulkanR {
        class VulkanContext;
        class VulkanDevice;
    }
}

namespace AstralEngine {

    struct FrameSyncObjects {
        VkSemaphore imageAvailableSemaphore;
        VkSemaphore renderFinishedSemaphore;
        VkFence inFlightFence;
    };

    class Renderer {
    public:
        explicit Renderer(Window& window);
        ~Renderer();

        // Non-copyable and non-movable for simplicity
        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;

        void DrawScene(const ECS::Scene& scene, ImGuiDrawData* imguiData = nullptr);
        void RecreateSwapChain();
        
        // Expose Vulkan device for 3D systems
        VulkanR::VulkanDevice& getDevice() const;

    private:
        class RendererImpl;
        std::unique_ptr<RendererImpl> m_impl;
    };

}