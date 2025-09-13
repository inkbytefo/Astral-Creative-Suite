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
        
        explicit Window(const WindowConfig& config = WindowConfig::fromEngineConfig());
        
        Window(const std::string& title, uint32_t width, uint32_t height);
        
        ~Window();
        
        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;
        Window(Window&&) = delete;
        Window& operator=(Window&&) = delete;
        
        void pollEvents();
        void swapBuffers();
        void close();
        
        bool shouldClose() const;
        bool isMinimized() const;
        bool isFocused() const;
        bool isFullscreen() const;
        
        uint32_t getWidth() const;
        uint32_t getHeight() const;
        std::pair<uint32_t, uint32_t> getSize() const;
        
        int32_t getPositionX() const;
        int32_t getPositionY() const;
        std::pair<int32_t, int32_t> getPosition() const;
        
        const std::string& getTitle() const;
        
        void setSize(uint32_t width, uint32_t height);
        void setPosition(int32_t x, int32_t y);
        void setTitle(const std::string& title);
        void setFullscreen(bool fullscreen);
        void setVSync(bool enabled);
        
        void setEventCallback(const EventCallbackFn& callback);
        
        SDL_Window* getNativeWindow() const;
        void* getNativeHandle() const;
        
        float getAspectRatio() const;
        void center();
        void maximize();
        void minimize();
        void restore();
        
        void logWindowInfo() const;
        
    private:
        class WindowImpl;
        std::unique_ptr<WindowImpl> m_impl;
    };
    
} // namespace AstralEngine
