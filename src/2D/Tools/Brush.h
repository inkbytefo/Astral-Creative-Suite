#pragma once

#include "ECS/Components.h"
#include "2D/Layers/Layer.h"
#include <glm/glm.hpp>
#include <vector>
#include <memory>

namespace AstralEngine {
    namespace D2 {
        // Brush types
        enum class BrushType {
            Round,
            Square,
            SoftRound,
            HardRound,
            Texture
        };
        
        // Brush tool component
        struct BrushTool : public ECS::IComponent {
            float size = 10.0f;
            float hardness = 1.0f;
            float opacity = 1.0f;
            glm::vec4 color = {0.0f, 0.0f, 0.0f, 1.0f};
            BrushType type = BrushType::Round;
            
            // Spacing for stroke rendering
            float spacing = 0.25f;
            
            // Texture for texture brushes
            // std::shared_ptr<Texture> brushTexture;
            
            // Constructor
            BrushTool() = default;
            
            // Helper methods
            float getRadius() const { return size * 0.5f; }
        };
        
        // Brush stroke structure
        struct BrushStroke {
            std::vector<glm::vec2> points;
            float pressure = 1.0f;
            float tiltX = 0.0f;
            float tiltY = 0.0f;
            float timestamp = 0.0f;
        };
        
        // Brush system for painting operations
        class BrushSystem {
        public:
            BrushSystem(ECS::Scene& scene);
            ~BrushSystem() = default;
            
            // Brush operations
            void beginStroke(const glm::vec2& startPoint);
            void addPointToStroke(const glm::vec2& point);
            void endStroke();
            
            // Paint operations
            void paintStroke(ECS::EntityID layerId, const BrushStroke& stroke);
            void eraseStroke(ECS::EntityID layerId, const BrushStroke& stroke);
            
            // Brush dynamics
            void applyPressure(float pressure);
            void applyTilt(const glm::vec2& tilt);
            
            // Rendering
            void renderBrushPreview(const glm::vec2& position);
            
            // Get current brush
            BrushTool& getCurrentBrush() { return m_currentBrush; }
            const BrushTool& getCurrentBrush() const { return m_currentBrush; }
            
        private:
            ECS::Scene& m_scene;
            BrushTool m_currentBrush;
            BrushStroke m_currentStroke;
            bool m_isDrawing = false;
            
            // Internal methods
            void rasterizeStroke(ECS::EntityID layerId, const BrushStroke& stroke);
            glm::vec4 sampleBrushTexture(const glm::vec2& uv);
        };
    }
}