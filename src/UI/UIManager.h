#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include "Core/Logger.h"
#include "Events/Events.h"

namespace AstralEngine {
    namespace UI {
        // Forward declarations
        class Window;
        
        /**
         * @brief UIManager class for managing the user interface of the creative suite
         * 
         * This class provides a framework for creating and managing UI windows and widgets
         * for the 2D graphics editor.
         */
        class UIManager {
        public:
            UIManager();
            ~UIManager();
            
            // Non-copyable
            UIManager(const UIManager&) = delete;
            UIManager& operator=(const UIManager&) = delete;
            
            // Initialize the UI system
            void initialize();
            
            // Shutdown the UI system
            void shutdown();
            
            // Render all UI elements
            void render();
            
            // Handle input events
            void handleEvent(Event& event);
            
            // Window management
            void addWindow(const std::string& name, std::function<void()> renderFunc);
            void removeWindow(const std::string& name);
            void showWindow(const std::string& name);
            void hideWindow(const std::string& name);
            
            // Widget rendering functions
            bool button(const std::string& label);
            bool checkbox(const std::string& label, bool* value);
            bool sliderFloat(const std::string& label, float* value, float min, float max);
            bool colorPicker(const std::string& label, glm::vec4* color);
            
            // Menu functions
            bool beginMainMenuBar();
            void endMainMenuBar();
            bool beginMenu(const std::string& label);
            void endMenu();
            bool menuItem(const std::string& label, const std::string& shortcut = "");
            
        private:
            std::vector<std::unique_ptr<Window>> m_windows;
            bool m_initialized = false;
        };
        
        /**
         * @brief Window class for UI windows
         */
        class Window {
        public:
            Window(const std::string& name, std::function<void()> renderFunc);
            ~Window() = default;
            
            void render();
            void show() { m_visible = true; }
            void hide() { m_visible = false; }
            bool isVisible() const { return m_visible; }
            const std::string& getName() const { return m_name; }
            
        private:
            std::string m_name;
            std::function<void()> m_renderFunc;
            bool m_visible = true;
        };
    }
}