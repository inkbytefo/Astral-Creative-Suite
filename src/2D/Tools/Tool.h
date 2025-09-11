#pragma once

#include "ECS/Components.h"
#include "Events/InputEvent.h"
#include "2D/Canvas/Canvas.h"
#include "2D/Layers/Layer.h"
#include "2D/Tools/Brush.h"
#include <glm/glm.hpp>
#include <string>
#include <memory>

namespace AstralEngine {
    namespace D2 {
        // Base tool class
        class Tool {
        public:
            Tool(const std::string& name) : m_name(name) {}
            virtual ~Tool() = default;
            
            // Tool lifecycle
            virtual void activate() {}
            virtual void deactivate() {}
            
            // Input handling
            virtual void onMouseDown(const glm::vec2& position, ECS::EntityID canvasId) {}
            virtual void onMouseUp(const glm::vec2& position, ECS::EntityID canvasId) {}
            virtual void onMouseMove(const glm::vec2& position, ECS::EntityID canvasId) {}
            virtual void onKeyDown(int key) {}
            virtual void onKeyUp(int key) {}
            
            // Rendering
            virtual void render() {}
            
            // Getters
            const std::string& getName() const { return m_name; }
            virtual bool isActive() const { return m_active; }
            
        protected:
            std::string m_name;
            bool m_active = false;
        };
        
        // Selection tool
        class SelectionTool : public Tool {
        public:
            SelectionTool() : Tool("Selection") {}
            
            void onMouseDown(const glm::vec2& position, ECS::EntityID canvasId) override;
            void onMouseUp(const glm::vec2& position, ECS::EntityID canvasId) override;
            void onMouseMove(const glm::vec2& position, ECS::EntityID canvasId) override;
            void render() override;
            
        private:
            bool m_selecting = false;
            glm::vec2 m_startPos;
            glm::vec2 m_endPos;
        };
        
        // Brush tool
        class BrushTool2D : public Tool {
        public:
            BrushTool2D(BrushSystem& brushSystem) : Tool("Brush"), m_brushSystem(brushSystem) {}
            
            void activate() override;
            void deactivate() override;
            void onMouseDown(const glm::vec2& position, ECS::EntityID canvasId) override;
            void onMouseUp(const glm::vec2& position, ECS::EntityID canvasId) override;
            void onMouseMove(const glm::vec2& position, ECS::EntityID canvasId) override;
            void render() override;
            
        private:
            BrushSystem& m_brushSystem;
            bool m_drawing = false;
        };
        
        // Eraser tool
        class EraserTool : public Tool {
        public:
            EraserTool(BrushSystem& brushSystem) : Tool("Eraser"), m_brushSystem(brushSystem) {}
            
            void activate() override;
            void deactivate() override;
            void onMouseDown(const glm::vec2& position, ECS::EntityID canvasId) override;
            void onMouseUp(const glm::vec2& position, ECS::EntityID canvasId) override;
            void onMouseMove(const glm::vec2& position, ECS::EntityID canvasId) override;
            void render() override;
            
        private:
            BrushSystem& m_brushSystem;
            bool m_erasing = false;
        };
        
        // Tool manager
        class ToolManager {
        public:
            ToolManager(ECS::Scene& scene, CanvasSystem& canvasSystem, BrushSystem& brushSystem);
            ~ToolManager();
            
            // Tool management
            void registerTool(std::unique_ptr<Tool> tool);
            void selectTool(const std::string& toolName);
            void deselectTool();
            
            // Input handling
            void handleInput(const InputEvent& event, ECS::EntityID canvasId);
            
            // Rendering
            void render();
            
            // Getters
            Tool* getActiveTool() { return m_activeTool; }
            const Tool* getActiveTool() const { return m_activeTool; }
            
        private:
            ECS::Scene& m_scene;
            CanvasSystem& m_canvasSystem;
            BrushSystem& m_brushSystem;
            
            std::vector<std::unique_ptr<Tool>> m_tools;
            Tool* m_activeTool = nullptr;
        };
    }
}