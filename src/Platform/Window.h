#pragma once

#include "Core/Logger.h"
#include "Core/EngineConfig.h"
#include "Events/ApplicationEvent.h"
#include <string>
#include <cstdint>
#include <memory>
#include <functional>
#include <vector>

// Forward declarations to avoid SDL3 header pollution
struct SDL_Window;
union SDL_Event;

namespace AstralEngine {
    
    /**
     * @brief Window configuration structure
     */
    struct WindowConfig {
        std::string title = "AstralEngine";
        uint32_t width = 1920;
        uint32_t height = 1080;
        bool fullscreen = false;
        bool resizable = true;
        bool vsync = true;
        bool decorated = true;
        bool focused = true;
        int32_t positionX = -1; // -1 means centered
        int32_t positionY = -1; // -1 means centered
        
        // Create from engine config
        static WindowConfig fromEngineConfig();
        
        // Validation
        bool isValid() const;
    };
    
    // Window events are now defined in Events/ApplicationEvent.h
    
    /**
     * @brief Platform-agnostic Window class
     * 
     * This class provides a modern, event-driven window management system
     * that integrates seamlessly with the AstralEngine architecture.
     * 
     * Features:
     * - Event-driven architecture (no direct renderer coupling)
     * - Configuration-based initialization
     * - Proper resource management (RAII)
     * - Thread-safe event handling
     * - Platform abstraction
     * - Memory-efficient event system
     */
    class Window {
    public:
        using EventCallbackFn = std::function<void(Event&)>;
        
        /**
         * @brief Construct window with configuration
         * @param config Window configuration parameters
         */
        explicit Window(const WindowConfig& config = WindowConfig::fromEngineConfig());
        
        /**
         * @brief Legacy constructor for backward compatibility
         */
        Window(const std::string& title, uint32_t width, uint32_t height);
        
        ~Window();
        
        // Non-copyable and non-movable
        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;
        Window(Window&&) = delete;
        Window& operator=(Window&&) = delete;
        
        // Core window operations
        void pollEvents();
        void swapBuffers(); // For explicit buffer swapping if needed
        void close();
        
        // State queries
        bool shouldClose() const { return m_shouldClose; }
        bool isMinimized() const;
        bool isFocused() const { return m_focused; }
        bool isFullscreen() const { return m_fullscreen; }
        
        // Window properties
        uint32_t getWidth() const { return m_width; }
        uint32_t getHeight() const { return m_height; }
        std::pair<uint32_t, uint32_t> getSize() const { return {m_width, m_height}; }
        
        int32_t getPositionX() const;
        int32_t getPositionY() const;
        std::pair<int32_t, int32_t> getPosition() const;
        
        const std::string& getTitle() const { return m_title; }
        
        // Window modifications
        void setSize(uint32_t width, uint32_t height);
        void setPosition(int32_t x, int32_t y);
        void setTitle(const std::string& title);
        void setFullscreen(bool fullscreen);
        void setVSync(bool enabled);
        
        // Event system
        void setEventCallback(const EventCallbackFn& callback) { m_eventCallback = callback; }
        
        // Native handle access (for Vulkan/platform-specific code)
        SDL_Window* getNativeWindow() const { return m_window; }
        void* getNativeHandle() const; // Platform-specific native handle
        
        // Utility functions
        float getAspectRatio() const { return static_cast<float>(m_width) / static_cast<float>(m_height); }
        void center(); // Center window on screen
        void maximize();
        void minimize();
        void restore();
        
        // Debug/diagnostics
        void logWindowInfo() const;
        
    private:
        // Initialization and cleanup
        void initSDL();
        void createWindow(const WindowConfig& config);
        void shutdownSDL();
        
        // Event processing
        void processEvent(const SDL_Event& event);
        void dispatchEvent(Event& event);
        
        // Window state updates
        void updateWindowProperties();
        
        // SDL window handle
        SDL_Window* m_window = nullptr;
        
        // Window properties
        std::string m_title;
        uint32_t m_width = 0;
        uint32_t m_height = 0;
        bool m_fullscreen = false;
        bool m_resizable = true;
        bool m_decorated = true;
        bool m_vsync = true;
        
        // Window state
        bool m_shouldClose = false;
        bool m_focused = true;
        bool m_initialized = false;
        
        // Event system
        EventCallbackFn m_eventCallback;
        
        // SDL initialization tracking (static to avoid multiple init/quit)
        static bool s_sdlInitialized;
        static uint32_t s_windowCount;
    };
    
} // namespace AstralEngine
