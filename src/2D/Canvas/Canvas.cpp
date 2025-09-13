#include "2D/Canvas/Canvas.h"
#include "Core/Logger.h"
#include "2D/Layers/Layer.h"
#include "Renderer/RRenderer.h"
#include "Renderer/Shader.h"
#include "Renderer/Model.h"
#include <glm/gtc/matrix_transform.hpp>

namespace AstralEngine {
    namespace D2 {
        glm::mat3 Canvas::getViewMatrix() const {
            // Create view matrix for 2D canvas
            glm::mat3 translation = glm::translate(glm::mat3(1.0f), offset);
            glm::mat3 scale = glm::scale(glm::mat3(1.0f), glm::vec2(zoom, zoom));
            
            // Combine transformations: T * S
            return translation * scale;
        }
        
        glm::vec2 Canvas::screenToWorld(const glm::vec2& screenPos) const {
            // Convert screen coordinates to world coordinates
            glm::vec2 normalized = screenPos; // TODO: Normalize based on screen size
            glm::vec3 worldPos = glm::inverse(getViewMatrix()) * glm::vec3(normalized, 1.0f);
            return glm::vec2(worldPos.x, worldPos.y);
        }
        
        glm::vec2 Canvas::worldToScreen(const glm::vec2& worldPos) const {
            // Convert world coordinates to screen coordinates
            glm::vec3 screenPos = getViewMatrix() * glm::vec3(worldPos, 1.0f);
            return glm::vec2(screenPos.x, screenPos.y);
        }
        
        CanvasSystem::CanvasSystem(ECS::Scene& scene) 
            : m_scene(scene), m_renderer(nullptr) {
            AE_INFO("CanvasSystem başlatıldı (Renderer olmadan)");
        }

        CanvasSystem::CanvasSystem(ECS::Scene& scene, Renderer& renderer) 
            : m_scene(scene), m_renderer(&renderer) {
            AE_INFO("CanvasSystem başlatıldı");
        }
        
        ECS::EntityID CanvasSystem::createCanvas(uint32_t width, uint32_t height) {
            // Create a new entity for the canvas
            ECS::EntityID canvasId = m_scene.createEntity("Canvas");
            
            // Add the canvas component
            auto& canvas = m_scene.addComponent<Canvas>(canvasId);
            canvas.width = width;
            canvas.height = height;
            
            AE_DEBUG("Canvas oluşturuldu: {}x{} (ID: {})", width, height, canvasId);
            return canvasId;
        }
        
        void CanvasSystem::resizeCanvas(ECS::EntityID canvasId, uint32_t width, uint32_t height) {
            // Check if the canvas exists
            if (!m_scene.hasComponent<Canvas>(canvasId)) {
                AE_WARN("Canvas bulunamadı: {}", canvasId);
                return;
            }
            
            // Resize the canvas
            auto& canvas = m_scene.getComponent<Canvas>(canvasId);
            canvas.width = width;
            canvas.height = height;
            
            AE_DEBUG("Canvas yeniden boyutlandırıldı: {}x{} (ID: {})", width, height, canvasId);
        }
        
        void CanvasSystem::renderCanvas(ECS::EntityID canvasId, const std::vector<ECS::EntityID>& layers) {
            // Check if the canvas exists
            if (!m_scene.hasComponent<Canvas>(canvasId)) {
                AE_WARN("Canvas bulunamadı: {}", canvasId);
                return;
            }
            
            // Get the canvas
            const auto& canvas = m_scene.getComponent<Canvas>(canvasId);
            
            // For now, we'll just log the operation
            // In a full implementation, we would:
            // 1. Clear the framebuffer with the background color
            // 2. Render each layer in order
            // 3. Apply layer blending modes
            // 4. Render grid if enabled
            // 5. Render any UI elements on top
            
            AE_DEBUG("Canvas render ediliyor: {}x{} (ID: {}), {} layer", 
                     canvas.width, canvas.height, canvasId, layers.size());
            
            // Render grid if enabled
            if (canvas.showGrid) {
                renderGrid(canvasId);
            }
        }
        
        void CanvasSystem::handleInput(const InputEvent& event, ECS::EntityID canvasId) {
            // Check if the canvas exists
            if (!m_scene.hasComponent<Canvas>(canvasId)) {
                AE_WARN("Canvas bulunamadı: {}", canvasId);
                return;
            }
            
            // Handle different input events
            switch (event.getType()) {
                case InputEventType::MouseScroll:
                    // Zoom with mouse wheel
                    // zoom(canvasId, event.getScrollDelta().y * 0.1f);
                    break;
                    
                case InputEventType::MouseMove:
                    // Pan with middle mouse button
                    // if (event.isMouseButtonDown(MouseButton::Middle)) {
                    //     pan(canvasId, event.getMouseDelta());
                    // }
                    break;
                    
                case InputEventType::MouseButtonPress:
                    // Handle mouse button presses
                    // if (event.getMouseButton() == MouseButton::Middle) {
                    //     // Middle mouse button for panning
                    // }
                    break;
                    
                default:
                    break;
            }
            
            AE_DEBUG("Canvas input event işlendi: {} (ID: {})", 
                     static_cast<int>(event.getType()), canvasId);
        }
        
        void CanvasSystem::zoom(ECS::EntityID canvasId, float delta) {
            // Check if the canvas exists
            if (!m_scene.hasComponent<Canvas>(canvasId)) {
                AE_WARN("Canvas bulunamadı: {}", canvasId);
                return;
            }
            
            // Get the canvas
            auto& canvas = m_scene.getComponent<Canvas>(canvasId);
            
            // Apply zoom (clamp between 0.1 and 10.0)
            canvas.zoom = glm::clamp(canvas.zoom + delta, 0.1f, 10.0f);
            
            // Update canvas transform
            updateCanvasTransform(canvasId);
            
            AE_DEBUG("Canvas zoom uygulandı: {} (ID: {})", canvas.zoom, canvasId);
        }
        
        void CanvasSystem::pan(ECS::EntityID canvasId, const glm::vec2& delta) {
            // Check if the canvas exists
            if (!m_scene.hasComponent<Canvas>(canvasId)) {
                AE_WARN("Canvas bulunamadı: {}", canvasId);
                return;
            }
            
            // Get the canvas
            auto& canvas = m_scene.getComponent<Canvas>(canvasId);
            
            // Apply pan
            canvas.offset += delta * (1.0f / canvas.zoom);
            
            // Update canvas transform
            updateCanvasTransform(canvasId);
            
            AE_DEBUG("Canvas pan uygulandı: ({}, {}) (ID: {})", 
                     delta.x, delta.y, canvasId);
        }
        
        void CanvasSystem::resetView(ECS::EntityID canvasId) {
            // Check if the canvas exists
            if (!m_scene.hasComponent<Canvas>(canvasId)) {
                AE_WARN("Canvas bulunamadı: {}", canvasId);
                return;
            }
            
            // Get the canvas
            auto& canvas = m_scene.getComponent<Canvas>(canvasId);
            
            // Reset view properties
            canvas.offset = {0, 0};
            canvas.zoom = 1.0f;
            
            // Update canvas transform
            updateCanvasTransform(canvasId);
            
            AE_DEBUG("Canvas view sıfırlandı (ID: {})", canvasId);
        }
        
        void CanvasSystem::setShowGrid(ECS::EntityID canvasId, bool show) {
            // Check if the canvas exists
            if (!m_scene.hasComponent<Canvas>(canvasId)) {
                AE_WARN("Canvas bulunamadı: {}", canvasId);
                return;
            }
            
            // Get the canvas
            auto& canvas = m_scene.getComponent<Canvas>(canvasId);
            
            // Set grid visibility
            canvas.showGrid = show;
            
            AE_DEBUG("Canvas grid {} (ID: {})", show ? "gösteriliyor" : "gizleniyor", canvasId);
        }
        
        void CanvasSystem::setGridSize(ECS::EntityID canvasId, uint32_t size) {
            // Check if the canvas exists
            if (!m_scene.hasComponent<Canvas>(canvasId)) {
                AE_WARN("Canvas bulunamadı: {}", canvasId);
                return;
            }
            
            // Get the canvas
            auto& canvas = m_scene.getComponent<Canvas>(canvasId);
            
            // Set grid size
            canvas.gridSize = size;
            
            AE_DEBUG("Canvas grid boyutu ayarlandı: {} (ID: {})", size, canvasId);
        }
        
        void CanvasSystem::updateCanvasTransform(ECS::EntityID canvasId) {
            // Check if the canvas exists
            if (!m_scene.hasComponent<Canvas>(canvasId)) {
                AE_WARN("Canvas bulunamadı: {}", canvasId);
                return;
            }
            
            // In a full implementation, we would update any cached transformation matrices
            // or notify systems that depend on the canvas transform
            
            AE_DEBUG("Canvas transform güncellendi (ID: {})", canvasId);
        }
        
        void CanvasSystem::renderGrid(ECS::EntityID canvasId) {
            // Check if the canvas exists
            if (!m_scene.hasComponent<Canvas>(canvasId)) {
                AE_WARN("Canvas bulunamadı: {}", canvasId);
                return;
            }
            
            // Get the canvas
            const auto& canvas = m_scene.getComponent<Canvas>(canvasId);
            
            // For now, we'll just log the operation
            // In a full implementation, we would render grid lines using the renderer
            
            AE_DEBUG("Canvas grid render ediliyor: {}px (ID: {})", canvas.gridSize, canvasId);
        }
    }
}