#include "2D/Tools/Brush.h"
#include "Core/Logger.h"
#include "Renderer/Renderer.h"
#include "Renderer/Texture.h"
#include "2D/Layers/Layer.h"
#include <algorithm>
#include <cmath>

namespace AstralEngine {
    namespace D2 {
        BrushSystem::BrushSystem(ECS::Scene& scene) : m_scene(scene) {
            AE_INFO("BrushSystem başlatıldı");
        }
        
        void BrushSystem::beginStroke(const glm::vec2& startPoint) {
            if (m_isDrawing) {
                AE_WARN("Zaten bir fırça darbesi devam ediyor");
                return;
            }
            
            m_currentStroke.points.clear();
            m_currentStroke.points.push_back(startPoint);
            m_currentStroke.pressure = 1.0f;
            m_currentStroke.tiltX = 0.0f;
            m_currentStroke.tiltY = 0.0f;
            m_currentStroke.timestamp = 0.0f; // TODO: Use actual timestamp
            
            m_isDrawing = true;
            
            AE_DEBUG("Fırça darbesi başlatıldı: ({}, {})", startPoint.x, startPoint.y);
        }
        
        void BrushSystem::addPointToStroke(const glm::vec2& point) {
            if (!m_isDrawing) {
                AE_WARN("Devam eden bir fırça darbesi yok");
                return;
            }
            
            m_currentStroke.points.push_back(point);
            
            AE_DEBUG("Fırça darbesine nokta eklendi: ({}, {})", point.x, point.y);
        }
        
        void BrushSystem::endStroke() {
            if (!m_isDrawing) {
                AE_WARN("Devam eden bir fırça darbesi yok");
                return;
            }
            
            m_isDrawing = false;
            
            AE_DEBUG("Fırça darbesi sonlandırıldı, {} nokta", m_currentStroke.points.size());
        }
        
        void BrushSystem::paintStroke(ECS::EntityID layerId, const BrushStroke& stroke) {
            // Check if the layer exists
            if (!m_scene.hasComponent<Layer>(layerId)) {
                AE_WARN("Layer bulunamadı: {}", layerId);
                return;
            }
            
            // For now, we'll just log the operation
            // In a full implementation, we would actually paint on the layer's texture
            AE_DEBUG("Fırça darbesi uygulanıyor: {} nokta, layer {}", 
                     stroke.points.size(), layerId);
            
            // Rasterize the stroke
            rasterizeStroke(layerId, stroke);
        }
        
        void BrushSystem::eraseStroke(ECS::EntityID layerId, const BrushStroke& stroke) {
            // Check if the layer exists
            if (!m_scene.hasComponent<Layer>(layerId)) {
                AE_WARN("Layer bulunamadı: {}", layerId);
                return;
            }
            
            // For now, we'll just log the operation
            // In a full implementation, we would actually erase from the layer's texture
            AE_DEBUG("Silgi darbesi uygulanıyor: {} nokta, layer {}", 
                     stroke.points.size(), layerId);
        }
        
        void BrushSystem::applyPressure(float pressure) {
            m_currentStroke.pressure = pressure;
            AE_DEBUG("Basınç uygulandı: {}", pressure);
        }
        
        void BrushSystem::applyTilt(const glm::vec2& tilt) {
            m_currentStroke.tiltX = tilt.x;
            m_currentStroke.tiltY = tilt.y;
            AE_DEBUG("Eğim uygulandı: ({}, {})", tilt.x, tilt.y);
        }
        
        void BrushSystem::rasterizeStroke(ECS::EntityID layerId, const BrushStroke& stroke) {
            // Check if the layer exists
            if (!m_scene.hasComponent<Layer>(layerId)) {
                AE_WARN("Layer bulunamadı: {}", layerId);
                return;
            }
            
            // Get the layer
            auto& layer = m_scene.getComponent<Layer>(layerId);
            
            // For now, we'll just log the operation
            // In a full implementation, we would:
            // 1. Create a brush mask based on the stroke points
            // 2. Apply the brush dynamics (pressure, tilt)
            // 3. Blend the brush with the layer's texture
            // 4. Update the layer's texture
            
            AE_DEBUG("Fırça darbesi rasterize ediliyor: {} nokta, layer {}", 
                     stroke.points.size(), layerId);
        }
        
        glm::vec4 BrushSystem::sampleBrushTexture(const glm::vec2& uv) {
            // For now, we'll just return a solid color
            // In a full implementation, we would sample from the brush texture
            return m_currentBrush.color;
        }
        
        void BrushSystem::renderBrushPreview(const glm::vec2& position) {
            // For now, we'll just log the operation
            // In a full implementation, we would render a brush preview at the given position
            AE_DEBUG("Fırça önizlemesi render ediliyor: ({}, {})", position.x, position.y);
        }
    }
}