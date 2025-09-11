#include "UI/UIManager.h"
#include "Renderer/Renderer.h"
#include <algorithm>

namespace AstralEngine {
    namespace UI {
        UIManager::UIManager() {
            // Constructor
        }
        
        UIManager::~UIManager() {
            if (m_initialized) {
                shutdown();
            }
        }
        
        void UIManager::initialize() {
            if (m_initialized) {
                AE_WARN("UIManager zaten başlatılmış");
                return;
            }
            
            AE_INFO("UIManager başlatılıyor...");
            m_initialized = true;
            AE_INFO("UIManager başarıyla başlatıldı");
        }
        
        void UIManager::shutdown() {
            if (!m_initialized) {
                AE_WARN("UIManager zaten kapatılmış");
                return;
            }
            
            AE_INFO("UIManager kapatılıyor...");
            m_windows.clear();
            m_initialized = false;
            AE_INFO("UIManager başarıyla kapatıldı");
        }
        
        void UIManager::render() {
            if (!m_initialized) {
                AE_ERROR("UIManager başlatılmamışken render edilemez");
                return;
            }
            
            // Begin main menu bar
            if (beginMainMenuBar()) {
                // File menu
                if (beginMenu("File")) {
                    menuItem("New", "Ctrl+N");
                    menuItem("Open", "Ctrl+O");
                    menuItem("Save", "Ctrl+S");
                    menuItem("Save As...", "Ctrl+Shift+S");
                    endMenu();
                }
                
                // Edit menu
                if (beginMenu("Edit")) {
                    menuItem("Undo", "Ctrl+Z");
                    menuItem("Redo", "Ctrl+Y");
                    menuItem("Cut", "Ctrl+X");
                    menuItem("Copy", "Ctrl+C");
                    menuItem("Paste", "Ctrl+V");
                    endMenu();
                }
                
                // Layer menu
                if (beginMenu("Layer")) {
                    menuItem("New Layer");
                    menuItem("Duplicate Layer");
                    menuItem("Delete Layer");
                    menuItem("Merge Layers");
                    endMenu();
                }
                
                // View menu
                if (beginMenu("View")) {
                    menuItem("Zoom In", "Ctrl++");
                    menuItem("Zoom Out", "Ctrl+-");
                    menuItem("Reset View", "Ctrl+0");
                    menuItem("Show Grid");
                    endMenu();
                }
                
                endMainMenuBar();
            }
            
            // Render all visible windows
            for (auto& window : m_windows) {
                if (window->isVisible()) {
                    window->render();
                }
            }
        }
        
        void UIManager::handleEvent(Event& event) {
            if (!m_initialized) {
                return;
            }
            
            // For now, we'll just log the events
            // In a real implementation, we would pass events to the UI framework
            AE_DEBUG("UI event: {}", event.getName());
        }
        
        void UIManager::addWindow(const std::string& name, std::function<void()> renderFunc) {
            if (!m_initialized) {
                AE_ERROR("UIManager başlatılmadan pencere eklenemez");
                return;
            }
            
            // Check if window with this name already exists
            auto it = std::find_if(m_windows.begin(), m_windows.end(),
                [&name](const std::unique_ptr<Window>& window) {
                    return window->getName() == name;
                });
                
            if (it != m_windows.end()) {
                AE_WARN("Pencere zaten mevcut: {}", name);
                return;
            }
            
            m_windows.push_back(std::make_unique<Window>(name, renderFunc));
            AE_DEBUG("Pencere eklendi: {}", name);
        }
        
        void UIManager::removeWindow(const std::string& name) {
            if (!m_initialized) {
                AE_ERROR("UIManager başlatılmadan pencere silinemez");
                return;
            }
            
            auto it = std::find_if(m_windows.begin(), m_windows.end(),
                [&name](const std::unique_ptr<Window>& window) {
                    return window->getName() == name;
                });
                
            if (it != m_windows.end()) {
                m_windows.erase(it);
                AE_DEBUG("Pencere silindi: {}", name);
            } else {
                AE_WARN("Pencere bulunamadı: {}", name);
            }
        }
        
        void UIManager::showWindow(const std::string& name) {
            if (!m_initialized) {
                return;
            }
            
            auto it = std::find_if(m_windows.begin(), m_windows.end(),
                [&name](const std::unique_ptr<Window>& window) {
                    return window->getName() == name;
                });
                
            if (it != m_windows.end()) {
                (*it)->show();
                AE_DEBUG("Pencere gösterildi: {}", name);
            } else {
                AE_WARN("Pencere bulunamadı: {}", name);
            }
        }
        
        void UIManager::hideWindow(const std::string& name) {
            if (!m_initialized) {
                return;
            }
            
            auto it = std::find_if(m_windows.begin(), m_windows.end(),
                [&name](const std::unique_ptr<Window>& window) {
                    return window->getName() == name;
                });
                
            if (it != m_windows.end()) {
                (*it)->hide();
                AE_DEBUG("Pencere gizlendi: {}", name);
            } else {
                AE_WARN("Pencere bulunamadı: {}", name);
            }
        }
        
        // Widget rendering functions (stub implementations)
        bool UIManager::button(const std::string& label) {
            // In a real implementation, this would render a button and return true when clicked
            AE_DEBUG("Button rendered: {}", label);
            return false;
        }
        
        bool UIManager::checkbox(const std::string& label, bool* value) {
            // In a real implementation, this would render a checkbox and update the value
            AE_DEBUG("Checkbox rendered: {}", label);
            return false;
        }
        
        bool UIManager::sliderFloat(const std::string& label, float* value, float min, float max) {
            // In a real implementation, this would render a slider and update the value
            AE_DEBUG("Slider rendered: {}", label);
            return false;
        }
        
        bool UIManager::colorPicker(const std::string& label, glm::vec4* color) {
            // In a real implementation, this would render a color picker and update the color
            AE_DEBUG("Color picker rendered: {}", label);
            return false;
        }
        
        // Menu functions (stub implementations)
        bool UIManager::beginMainMenuBar() {
            AE_DEBUG("Main menu bar başlıyor");
            return true;
        }
        
        void UIManager::endMainMenuBar() {
            AE_DEBUG("Main menu bar bitiyor");
        }
        
        bool UIManager::beginMenu(const std::string& label) {
            AE_DEBUG("Menu başlıyor: {}", label);
            return true;
        }
        
        void UIManager::endMenu() {
            AE_DEBUG("Menu bitiyor");
        }
        
        bool UIManager::menuItem(const std::string& label, const std::string& shortcut) {
            AE_DEBUG("Menu item: {} ({})", label, shortcut);
            return false;
        }
        
        // Window implementation
        Window::Window(const std::string& name, std::function<void()> renderFunc)
            : m_name(name), m_renderFunc(renderFunc) {
        }
        
        void Window::render() {
            // In a real implementation, this would render the window
            AE_DEBUG("Window rendering: {}", m_name);
            if (m_renderFunc) {
                m_renderFunc();
            }
        }
    }
}