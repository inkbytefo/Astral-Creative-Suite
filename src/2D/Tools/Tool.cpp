#include "2D/Tools/Tool.h"
#include "Core/Logger.h"
#include "Renderer/Renderer.h"
#include <algorithm>

namespace AstralEngine {
    namespace D2 {
        // SelectionTool implementation
        void SelectionTool::onMouseDown(const glm::vec2& position, ECS::EntityID canvasId) {
            m_selecting = true;
            m_startPos = position;
            m_endPos = position;
            AE_DEBUG("Selection tool mouse down at ({}, {})", position.x, position.y);
        }
        
        void SelectionTool::onMouseUp(const glm::vec2& position, ECS::EntityID canvasId) {
            m_selecting = false;
            m_endPos = position;
            AE_DEBUG("Selection tool mouse up at ({}, {})", position.x, position.y);
        }
        
        void SelectionTool::onMouseMove(const glm::vec2& position, ECS::EntityID canvasId) {
            if (m_selecting) {
                m_endPos = position;
                AE_DEBUG("Selection tool mouse move at ({}, {})", position.x, position.y);
            }
        }
        
        void SelectionTool::render() {
            if (m_selecting) {
                AE_DEBUG("Selection tool rendering selection rectangle");
                // In a full implementation, we would render the selection rectangle
                // For now, we'll just log that rendering would happen here
            }
        }
        
        // BrushTool2D implementation
        void BrushTool2D::activate() {
            Tool::activate();
            AE_DEBUG("Brush tool activated");
        }
        
        void BrushTool2D::deactivate() {
            Tool::deactivate();
            if (m_drawing) {
                m_brushSystem.endStroke();
                m_drawing = false;
            }
            AE_DEBUG("Brush tool deactivated");
        }
        
        void BrushTool2D::onMouseDown(const glm::vec2& position, ECS::EntityID canvasId) {
            m_drawing = true;
            m_brushSystem.beginStroke(position);
            AE_DEBUG("Brush tool mouse down at ({}, {})", position.x, position.y);
        }
        
        void BrushTool2D::onMouseUp(const glm::vec2& position, ECS::EntityID canvasId) {
            if (m_drawing) {
                m_brushSystem.addPointToStroke(position);
                m_brushSystem.endStroke();
                m_drawing = false;
                AE_DEBUG("Brush tool mouse up at ({}, {})", position.x, position.y);
            }
        }
        
        void BrushTool2D::onMouseMove(const glm::vec2& position, ECS::EntityID canvasId) {
            if (m_drawing) {
                m_brushSystem.addPointToStroke(position);
                AE_DEBUG("Brush tool mouse move at ({}, {})", position.x, position.y);
            }
        }
        
        void BrushTool2D::render() {
            // In a full implementation, we would render brush previews
            AE_DEBUG("Brush tool rendering");
        }
        
        // EraserTool implementation
        void EraserTool::activate() {
            Tool::activate();
            AE_DEBUG("Eraser tool activated");
        }
        
        void EraserTool::deactivate() {
            Tool::deactivate();
            if (m_erasing) {
                m_brushSystem.endStroke();
                m_erasing = false;
            }
            AE_DEBUG("Eraser tool deactivated");
        }
        
        void EraserTool::onMouseDown(const glm::vec2& position, ECS::EntityID canvasId) {
            m_erasing = true;
            m_brushSystem.beginStroke(position);
            AE_DEBUG("Eraser tool mouse down at ({}, {})", position.x, position.y);
        }
        
        void EraserTool::onMouseUp(const glm::vec2& position, ECS::EntityID canvasId) {
            if (m_erasing) {
                m_brushSystem.addPointToStroke(position);
                m_brushSystem.endStroke();
                m_erasing = false;
                AE_DEBUG("Eraser tool mouse up at ({}, {})", position.x, position.y);
            }
        }
        
        void EraserTool::onMouseMove(const glm::vec2& position, ECS::EntityID canvasId) {
            if (m_erasing) {
                m_brushSystem.addPointToStroke(position);
                AE_DEBUG("Eraser tool mouse move at ({}, {})", position.x, position.y);
            }
        }
        
        void EraserTool::render() {
            // In a full implementation, we would render eraser previews
            AE_DEBUG("Eraser tool rendering");
        }
        
        // ToolManager implementation
        ToolManager::ToolManager(ECS::Scene& scene, CanvasSystem& canvasSystem, BrushSystem& brushSystem)
            : m_scene(scene), m_canvasSystem(canvasSystem), m_brushSystem(brushSystem) {
            AE_INFO("ToolManager başlatıldı");
        }
        
        ToolManager::~ToolManager() {
            AE_INFO("ToolManager kapatıldı");
        }
        
        void ToolManager::registerTool(std::unique_ptr<Tool> tool) {
            // Check if a tool with the same name already exists
            const std::string& toolName = tool->getName();
            auto it = std::find_if(m_tools.begin(), m_tools.end(),
                [&toolName](const std::unique_ptr<Tool>& existingTool) {
                    return existingTool->getName() == toolName;
                });
                
            if (it != m_tools.end()) {
                AE_WARN("Araç zaten kayıtlı: {}", toolName);
                return;
            }
            
            m_tools.push_back(std::move(tool));
            AE_DEBUG("Araç kaydedildi: {}", toolName);
        }
        
        void ToolManager::selectTool(const std::string& toolName) {
            // Find the tool
            auto it = std::find_if(m_tools.begin(), m_tools.end(),
                [&toolName](const std::unique_ptr<Tool>& tool) {
                    return tool->getName() == toolName;
                });
                
            if (it == m_tools.end()) {
                AE_WARN("Araç bulunamadı: {}", toolName);
                return;
            }
            
            // Deactivate current tool
            if (m_activeTool) {
                m_activeTool->deactivate();
            }
            
            // Activate new tool
            m_activeTool = it->get();
            m_activeTool->activate();
            
            AE_DEBUG("Araç seçildi: {}", toolName);
        }
        
        void ToolManager::deselectTool() {
            if (m_activeTool) {
                m_activeTool->deactivate();
                m_activeTool = nullptr;
                AE_DEBUG("Araç seçimi kaldırıldı");
            }
        }
        
        void ToolManager::handleInput(const InputEvent& event, ECS::EntityID canvasId) {
            if (!m_activeTool) {
                return;
            }
            
            // Handle different input events
            switch (event.getType()) {
                case InputEventType::MouseButtonDown:
                    {
                        // Convert screen position to world position
                        // For now, we'll just use the screen position
                        glm::vec2 position = event.getMousePosition();
                        m_activeTool->onMouseDown(position, canvasId);
                    }
                    break;
                    
                case InputEventType::MouseButtonUp:
                    {
                        // Convert screen position to world position
                        // For now, we'll just use the screen position
                        glm::vec2 position = event.getMousePosition();
                        m_activeTool->onMouseUp(position, canvasId);
                    }
                    break;
                    
                case InputEventType::MouseMove:
                    {
                        // Convert screen position to world position
                        // For now, we'll just use the screen position
                        glm::vec2 position = event.getMousePosition();
                        m_activeTool->onMouseMove(position, canvasId);
                    }
                    break;
                    
                case InputEventType::KeyDown:
                    m_activeTool->onKeyDown(event.getKeyCode());
                    break;
                    
                case InputEventType::KeyUp:
                    m_activeTool->onKeyUp(event.getKeyCode());
                    break;
                    
                default:
                    break;
            }
        }
        
        void ToolManager::render() {
            if (m_activeTool) {
                m_activeTool->render();
            }
        }
    }
}