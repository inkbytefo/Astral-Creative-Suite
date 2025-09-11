#pragma once

#include "Renderer.h"
#include "2D/CanvasRenderer.h"
#include "3D/SceneRenderer.h"
#include "Vulkan/VulkanContext.h"
#include "Vulkan/VulkanDevice.h"
#include "Vulkan/VulkanSwapChain.h"
#include <memory>

namespace AstralEngine {
    
    /**
     * @brief RenderSystem manages all rendering operations for both 2D and 3D
     * 
     * This class acts as the central coordinator for all rendering operations,
     * managing both 2D and 3D rendering contexts and providing a unified interface
     * for the creative suite.
     */
    class RenderSystem {
    public:
        explicit RenderSystem(Window& window);
        ~RenderSystem();

        // Non-copyable but movable
        RenderSystem(const RenderSystem&) = delete;
        RenderSystem& operator=(const RenderSystem&) = delete;
        RenderSystem(RenderSystem&&) noexcept = default;
        RenderSystem& operator=(RenderSystem&&) noexcept = default;

        // Initialization
        bool initialize();
        bool isInitialized() const;

        // Rendering operations
        void beginFrame();
        void endFrame();
        
        // 2D Rendering
        D2::CanvasRenderer& getCanvasRenderer();
        const D2::CanvasRenderer& getCanvasRenderer() const;
        
        // 3D Rendering
        D3::SceneRenderer& getSceneRenderer();
        const D3::SceneRenderer& getSceneRenderer() const;
        
        // Context management
        void onFramebufferResize();
        bool isFramebufferResized() const;
        
        // Device access
        Vulkan::VulkanDevice& getDevice();
        const Vulkan::VulkanDevice& getDevice() const;
        
        // Debug and statistics
        void printDebugInfo() const;
        void logMemoryUsage() const;

    private:
        class RenderSystemImpl;
        std::unique_ptr<RenderSystemImpl> m_impl;
    };

} // namespace AstralEngine