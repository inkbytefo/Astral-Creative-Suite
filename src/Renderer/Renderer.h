#pragma once

#include "RenderSystem.h"
#include <memory>

// Forward Declarations
class Window;
namespace ECS { class Scene; }

namespace AstralEngine {
    
    // Opaque extent type to avoid exposing Vulkan types in header
    struct SwapChainExtent {
        uint32_t width;
        uint32_t height;
    };

    /**
     * @brief Backward-compatible Renderer class
     * 
     * This class maintains backward compatibility with existing code
     * while delegating to the new RenderSystem implementation.
     */
    class Renderer {
    public:
        explicit Renderer(Window& window);
        ~Renderer();

        // Non-copyable but movable
        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;
        Renderer(Renderer&&) noexcept;
        Renderer& operator=(Renderer&&) noexcept;

        // Main rendering methods (backward compatibility)
        void drawFrame(const ECS::Scene& scene);
        void drawFrame(); // Backward compatibility with empty scene
        void onFramebufferResize();

        // Device access for external systems
        Vulkan::VulkanDevice& getDevice();
        const Vulkan::VulkanDevice& getDevice() const;

        // Renderer state queries
        bool isInitialized() const;
        bool isFramebufferResized() const;
        bool canRender() const;
        
        // Statistics and debug info
        size_t getSwapChainImageCount() const;
        SwapChainExtent getSwapChainExtent() const;
        void printDebugInfo() const;
        void logMemoryUsage() const;
        
        // Pipeline cache diagnostics
        void logPipelineCacheStats() const;
        void resetPipelineCacheStats();

    private:
        class RendererImpl;
        std::unique_ptr<RendererImpl> m_impl;
    };

} // namespace AstralEngine