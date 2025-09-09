#include "Window.h"
#include "Core/Logger.h"
#include "Core/EngineConfig.h"
#include "Events/ApplicationEvent.h"
#include <SDL3/SDL.h>
#include <stdexcept>
#include <cassert>

namespace AstralEngine {
    
    // Static member initialization
    bool Window::s_sdlInitialized = false;
    uint32_t Window::s_windowCount = 0;
    
    //=============================================================================
    // WindowConfig Implementation
    //=============================================================================
    
    WindowConfig WindowConfig::fromEngineConfig() {
        const auto& engineConfig = ENGINE_CONFIG();
        WindowConfig config;
        
        config.title = "AstralEngine";
        config.width = 1920;
        config.height = 1080;
        config.fullscreen = false;
        config.resizable = true;
        config.vsync = engineConfig.enableVSync;
        config.decorated = true;
        config.focused = true;
        config.positionX = -1; // Centered
        config.positionY = -1; // Centered
        
        return config;
    }
    
    bool WindowConfig::isValid() const {
        return width > 0 && width <= 16384 && 
               height > 0 && height <= 16384 &&
               !title.empty();
    }
    
    //=============================================================================
    // Window Implementation
    //=============================================================================
    
    Window::Window(const WindowConfig& config) {
        AE_DEBUG("Creating window with configuration: {}x{}, title: '{}'", 
                config.width, config.height, config.title);
        
        if (!config.isValid()) {
            throw std::invalid_argument("Invalid window configuration provided");
        }
        
        try {
            initSDL();
            createWindow(config);
            updateWindowProperties();
            m_initialized = true;
            
            AE_INFO("Window '{}' created successfully ({}x{})", m_title, m_width, m_height);
        } catch (const std::exception& e) {
            AE_ERROR("Failed to create window: {}", e.what());
            shutdownSDL();
            throw;
        }
    }
    
    Window::Window(const std::string& title, uint32_t width, uint32_t height) {
        // Legacy constructor - create config and initialize manually
        WindowConfig config;
        config.title = title;
        config.width = width;
        config.height = height;
        
        AE_DEBUG("Creating window with legacy constructor: {}x{}, title: '{}'", 
                width, height, title);
        
        if (!config.isValid()) {
            throw std::invalid_argument("Invalid window configuration provided");
        }
        
        try {
            initSDL();
            createWindow(config);
            updateWindowProperties();
            m_initialized = true;
            
            AE_INFO("Window '{}' created successfully ({}x{})", m_title, m_width, m_height);
        } catch (const std::exception& e) {
            AE_ERROR("Failed to create window: {}", e.what());
            shutdownSDL();
            throw;
        }
    }
    
    Window::~Window() {
        AE_DEBUG("Destroying window '{}'", m_title);
        
        if (m_window) {
            SDL_DestroyWindow(m_window);
            m_window = nullptr;
        }
        
        shutdownSDL();
        m_initialized = false;
        
        AE_INFO("Window '{}' destroyed successfully", m_title);
    }
    
    //=============================================================================
    // Core Window Operations
    //=============================================================================
    
    void Window::pollEvents() {
        if (!m_initialized) {
            AE_WARN("Attempting to poll events on uninitialized window");
            return;
        }
        
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            processEvent(event);
        }
    }
    
    void Window::swapBuffers() {
        // For Vulkan, this is typically handled by the swapchain
        // But we provide this for compatibility with OpenGL or other APIs
        if (m_window) {
            // SDL3 doesn't have a direct swap buffers for Vulkan
            // This would be used for OpenGL: SDL_GL_SwapWindow(m_window);
        }
    }
    
    void Window::close() {
        m_shouldClose = true;
        
        // Dispatch window close event
        if (m_eventCallback) {
            WindowCloseEvent event;
            m_eventCallback(event);
        }
    }
    
    //=============================================================================
    // State Queries
    //=============================================================================
    
    bool Window::isMinimized() const {
        if (!m_window) return false;
        
        Uint32 flags = SDL_GetWindowFlags(m_window);
        return (flags & SDL_WINDOW_MINIMIZED) != 0;
    }
    
    //=============================================================================
    // Window Properties
    //=============================================================================
    
    int32_t Window::getPositionX() const {
        if (!m_window) return 0;
        
        int x, y;
        SDL_GetWindowPosition(m_window, &x, &y);
        return x;
    }
    
    int32_t Window::getPositionY() const {
        if (!m_window) return 0;
        
        int x, y;
        SDL_GetWindowPosition(m_window, &x, &y);
        return y;
    }
    
    std::pair<int32_t, int32_t> Window::getPosition() const {
        if (!m_window) return {0, 0};
        
        int x, y;
        SDL_GetWindowPosition(m_window, &x, &y);
        return {x, y};
    }
    
    //=============================================================================
    // Window Modifications
    //=============================================================================
    
    void Window::setSize(uint32_t width, uint32_t height) {
        if (!m_window || width == 0 || height == 0) {
            AE_WARN("Invalid window size: {}x{}", width, height);
            return;
        }
        
        SDL_SetWindowSize(m_window, static_cast<int>(width), static_cast<int>(height));
        m_width = width;
        m_height = height;
        
        AE_DEBUG("Window size changed to {}x{}", width, height);
    }
    
    void Window::setPosition(int32_t x, int32_t y) {
        if (!m_window) return;
        
        SDL_SetWindowPosition(m_window, x, y);
        AE_DEBUG("Window position changed to ({}, {})", x, y);
    }
    
    void Window::setTitle(const std::string& title) {
        if (!m_window || title.empty()) return;
        
        SDL_SetWindowTitle(m_window, title.c_str());
        m_title = title;
        
        AE_DEBUG("Window title changed to '{}'", title);
    }
    
    void Window::setFullscreen(bool fullscreen) {
        if (!m_window || m_fullscreen == fullscreen) return;
        
        Uint32 flags = fullscreen ? SDL_WINDOW_FULLSCREEN : 0;
        if (SDL_SetWindowFullscreen(m_window, flags) == 0) {
            m_fullscreen = fullscreen;
            AE_INFO("Window fullscreen mode: {}", fullscreen ? "enabled" : "disabled");
        } else {
            AE_ERROR("Failed to set fullscreen mode: {}", SDL_GetError());
        }
    }
    
    void Window::setVSync(bool enabled) {
        // VSync is typically handled by the graphics API (Vulkan/OpenGL)
        // Store the preference for the graphics context to use
        m_vsync = enabled;
        AE_DEBUG("VSync preference set to: {}", enabled ? "enabled" : "disabled");
    }
    
    //=============================================================================
    // Utility Functions
    //=============================================================================
    
    void* Window::getNativeHandle() const {
        if (!m_window) return nullptr;
        
#ifdef _WIN32
        return SDL_GetPointerProperty(SDL_GetWindowProperties(m_window), "SDL.window.win32.hwnd", nullptr);
#elif defined(__linux__)
        return SDL_GetPointerProperty(SDL_GetWindowProperties(m_window), "SDL.window.x11.window", nullptr);
#elif defined(__APPLE__)
        return SDL_GetPointerProperty(SDL_GetWindowProperties(m_window), "SDL.window.cocoa.window", nullptr);
#else
        return nullptr;
#endif
    }
    
    void Window::center() {
        if (!m_window) return;
        
        SDL_SetWindowPosition(m_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        AE_DEBUG("Window centered on screen");
    }
    
    void Window::maximize() {
        if (!m_window) return;
        
        SDL_MaximizeWindow(m_window);
        AE_DEBUG("Window maximized");
    }
    
    void Window::minimize() {
        if (!m_window) return;
        
        SDL_MinimizeWindow(m_window);
        AE_DEBUG("Window minimized");
    }
    
    void Window::restore() {
        if (!m_window) return;
        
        SDL_RestoreWindow(m_window);
        AE_DEBUG("Window restored");
    }
    
    void Window::logWindowInfo() const {
        if (!m_initialized) {
            AE_INFO("Window not initialized");
            return;
        }
        
        AE_INFO("=== Window Information ===");
        AE_INFO("Title: '{}'", m_title);
        AE_INFO("Size: {}x{}", m_width, m_height);
        AE_INFO("Position: ({}, {})", getPositionX(), getPositionY());
        AE_INFO("Aspect Ratio: {:.3f}", getAspectRatio());
        AE_INFO("Fullscreen: {}", m_fullscreen ? "Yes" : "No");
        AE_INFO("Focused: {}", m_focused ? "Yes" : "No");
        AE_INFO("Minimized: {}", isMinimized() ? "Yes" : "No");
        AE_INFO("VSync: {}", m_vsync ? "Enabled" : "Disabled");
        AE_INFO("=========================");
    }
    
    //=============================================================================
    // Private Implementation
    //=============================================================================
    
    void Window::initSDL() {
        if (s_sdlInitialized) {
            s_windowCount++;
            return;
        }
        
        AE_DEBUG("Initializing SDL3 video subsystem...");
        
        // SDL3: SDL_Init() returns bool (true=success, false=failure)
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            throw std::runtime_error(
                std::string("Failed to initialize SDL3 video subsystem: ") + SDL_GetError()
            );
        }
        
        s_sdlInitialized = true;
        s_windowCount = 1;
        
        AE_INFO("SDL3 video subsystem initialized successfully");
    }
    
    void Window::createWindow(const WindowConfig& config) {
        // Build SDL window flags
        Uint32 flags = SDL_WINDOW_VULKAN;
        
        if (config.resizable) flags |= SDL_WINDOW_RESIZABLE;
        if (config.fullscreen) flags |= SDL_WINDOW_FULLSCREEN;
        if (!config.decorated) flags |= SDL_WINDOW_BORDERLESS;
        if (config.focused) flags |= SDL_WINDOW_INPUT_FOCUS;
        
        // Determine position
        int posX = (config.positionX == -1) ? SDL_WINDOWPOS_CENTERED : config.positionX;
        int posY = (config.positionY == -1) ? SDL_WINDOWPOS_CENTERED : config.positionY;
        
        // Create the window
        m_window = SDL_CreateWindow(
            config.title.c_str(),
            static_cast<int>(config.width),
            static_cast<int>(config.height),
            flags
        );
        
        if (!m_window) {
            throw std::runtime_error(
                std::string("Failed to create SDL window: ") + SDL_GetError()
            );
        }
        
        // Set position if specified
        if (config.positionX != -1 && config.positionY != -1) {
            SDL_SetWindowPosition(m_window, posX, posY);
        }
        
        // Store configuration
        m_title = config.title;
        m_width = config.width;
        m_height = config.height;
        m_fullscreen = config.fullscreen;
        m_resizable = config.resizable;
        m_decorated = config.decorated;
        m_vsync = config.vsync;
        m_focused = config.focused;
    }
    
    void Window::shutdownSDL() {
        if (!s_sdlInitialized) return;
        
        s_windowCount--;
        if (s_windowCount == 0) {
            AE_DEBUG("Shutting down SDL3 video subsystem...");
            SDL_Quit();
            s_sdlInitialized = false;
            AE_INFO("SDL3 video subsystem shutdown complete");
        }
    }
    
    void Window::processEvent(const SDL_Event& event) {
        switch (event.type) {
            case SDL_EVENT_QUIT: {
                AE_DEBUG("Window close requested by user");
                close();
                break;
            }
            
            case SDL_EVENT_WINDOW_RESIZED: {
                m_width = static_cast<uint32_t>(event.window.data1);
                m_height = static_cast<uint32_t>(event.window.data2);
                
                AE_DEBUG("Window resized to {}x{}", m_width, m_height);
                
                if (m_eventCallback) {
                    WindowResizeEvent resizeEvent(m_width, m_height);
                    dispatchEvent(resizeEvent);
                }
                break;
            }
            
            case SDL_EVENT_WINDOW_MOVED: {
                int32_t x = event.window.data1;
                int32_t y = event.window.data2;
                
                AE_DEBUG("Window moved to ({}, {})", x, y);
                
                if (m_eventCallback) {
                    WindowMovedEvent moveEvent(x, y);
                    dispatchEvent(moveEvent);
                }
                break;
            }
            
            case SDL_EVENT_WINDOW_FOCUS_GAINED: {
                m_focused = true;
                AE_DEBUG("Window gained focus");
                
                if (m_eventCallback) {
                    WindowFocusEvent focusEvent(true);
                    dispatchEvent(focusEvent);
                }
                break;
            }
            
            case SDL_EVENT_WINDOW_FOCUS_LOST: {
                m_focused = false;
                AE_DEBUG("Window lost focus");
                
                if (m_eventCallback) {
                    WindowLostFocusEvent focusEvent;
                    dispatchEvent(focusEvent);
                }
                break;
            }
            
            case SDL_EVENT_WINDOW_MINIMIZED: {
                AE_DEBUG("Window minimized");
                break;
            }
            
            case SDL_EVENT_WINDOW_RESTORED: {
                AE_DEBUG("Window restored");
                break;
            }
            
            case SDL_EVENT_WINDOW_MAXIMIZED: {
                AE_DEBUG("Window maximized");
                updateWindowProperties();
                break;
            }
            
            default:
                break;
        }
    }
    
    void Window::dispatchEvent(Event& event) {
        if (m_eventCallback) {
            m_eventCallback(event);
        }
    }
    
    void Window::updateWindowProperties() {
        if (!m_window) return;
        
        int width, height;
        SDL_GetWindowSize(m_window, &width, &height);
        m_width = static_cast<uint32_t>(width);
        m_height = static_cast<uint32_t>(height);
        
        Uint32 flags = SDL_GetWindowFlags(m_window);
        m_fullscreen = (flags & SDL_WINDOW_FULLSCREEN) != 0;
        m_focused = (flags & SDL_WINDOW_INPUT_FOCUS) != 0;
    }
    
} // namespace AstralEngine
