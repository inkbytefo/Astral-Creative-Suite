#pragma once

#include "../Vulkan/VulkanContext.h"
#include "ECS/Components.h"
#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace AstralEngine {
    namespace D2 {
        
        struct CanvasRenderData {
            uint32_t width;
            uint32_t height;
            glm::vec4 backgroundColor;
            glm::vec2 offset;
            float zoom;
            bool showGrid;
            uint32_t gridSize;
        };

        /**
         * @brief CanvasRenderer handles 2D canvas rendering operations
         * 
         * This class is responsible for rendering 2D canvases, layers, brushes,
         * and other 2D elements for the creative suite.
         */
        class CanvasRenderer {
        public:
            explicit CanvasRenderer(Vulkan::VulkanContext& context);
            ~CanvasRenderer();

            // Non-copyable but movable
            CanvasRenderer(const CanvasRenderer&) = delete;
            CanvasRenderer& operator=(const CanvasRenderer&) = delete;
            CanvasRenderer(CanvasRenderer&&) noexcept = default;
            CanvasRenderer& operator=(CanvasRenderer&&) noexcept = default;

            // Initialization
            bool initialize();
            bool isInitialized() const;

            // Canvas rendering
            void renderCanvas(const CanvasRenderData& canvasData, 
                            const std::vector<ECS::EntityID>& layers);
            
            // Grid rendering
            void renderGrid(const CanvasRenderData& canvasData);
            
            // Layer rendering
            void renderLayer(ECS::EntityID layerId);
            
            // Brush rendering
            void renderBrush(const glm::vec2& position, float size, const glm::vec4& color);
            
            // View management
            void setViewTransform(const glm::mat3& transform);
            glm::mat3 getViewTransform() const;
            
            // Resource management
            void cleanup();

        private:
            class CanvasRendererImpl;
            std::unique_ptr<CanvasRendererImpl> m_impl;
        };

    } // namespace D2
} // namespace AstralEngine