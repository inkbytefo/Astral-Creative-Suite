#pragma once

#include "ECS/Components.h"
#include "Renderer/RRenderer.h"
#include "Events/InputEvent.h"
#include "2D/Layers/Layer.h"
#include <glm/glm.hpp>

namespace AstralEngine {
    namespace D2 {
        // Canvas component for 2D workspace
        struct Canvas : public ECS::IComponent {
            uint32_t width = 1920;
            uint32_t height = 1080;
            glm::vec4 backgroundColor = {1.0f, 1.0f, 1.0f, 1.0f};
            
            // View properties
            glm::vec2 offset = {0, 0};
            float zoom = 1.0f;
            
            // Grid settings
            bool showGrid = false;
            uint32_t gridSize = 50;
            
            // Constructor
            Canvas() = default;
            
            // Constructor with dimensions
            Canvas(uint32_t w, uint32_t h) : width(w), height(h) {}
            
            // Helper methods
            glm::mat3 getViewMatrix() const;
            glm::vec2 screenToWorld(const glm::vec2& screenPos) const;
            glm::vec2 worldToScreen(const glm::vec2& worldPos) const;
        };
        
        // Canvas system for rendering and interaction
        class CanvasSystem {
        public:
            CanvasSystem(ECS::Scene& scene);
            CanvasSystem(ECS::Scene& scene, Renderer& renderer);
            ~CanvasSystem() = default;
            
            // Canvas operations
            ECS::EntityID createCanvas(uint32_t width, uint32_t height);
            void resizeCanvas(ECS::EntityID canvasId, uint32_t width, uint32_t height);
            
            // Rendering
            void renderCanvas(ECS::EntityID canvasId, const std::vector<ECS::EntityID>& layers);
            
            // Input handling
            void handleInput(const InputEvent& event, ECS::EntityID canvasId);
            
            // View manipulation
            void zoom(ECS::EntityID canvasId, float delta);
            void pan(ECS::EntityID canvasId, const glm::vec2& delta);
            void resetView(ECS::EntityID canvasId);
            
            // Grid settings
            void setShowGrid(ECS::EntityID canvasId, bool show);
            void setGridSize(ECS::EntityID canvasId, uint32_t size);
            
        private:
            ECS::Scene& m_scene;
            Renderer* m_renderer; // Changed to pointer
            
            // Internal methods
            void updateCanvasTransform(ECS::EntityID canvasId);
            void renderGrid(ECS::EntityID canvasId);
        };
    }
}