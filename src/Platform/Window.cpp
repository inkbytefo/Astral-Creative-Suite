#include "Window.h"
#include "Core/Logger.h"
#include "Core/EngineConfig.h"
#include "Events/ApplicationEvent.h"
#include <SDL3/SDL.h>
#include <stdexcept>
#include <cassert>
#include <memory>
#include <imgui_impl_sdl3.h>

namespace AstralEngine {

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
    // Pimpl (Private Implementation)
    //=============================================================================

    // Custom deleter for SDL_Window to be used with std::unique_ptr
    struct SdlWindowDeleter {
        void operator()(SDL_Window* w) const {
            if (w) {
                SDL_DestroyWindow(w);
            }
        }
    };

    class Window::WindowImpl {
    public:
        // Constructor
        WindowImpl(const WindowConfig& config) {
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

        // Destructor
        ~WindowImpl() {
            AE_DEBUG("Destroying window '{}'", m_title);
            // m_window unique_ptr automatically calls the deleter, which calls SDL_DestroyWindow
            shutdownSDL();
            m_initialized = false;
            AE_INFO("Window '{}' destroyed successfully", m_title);
        }

        // Public methods to be forwarded
        void pollEvents() {
            if (!m_initialized) {
                AE_WARN("Attempting to poll events on uninitialized window");
                return;
            }
            
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                ImGui_ImplSDL3_ProcessEvent(&event);
                processEvent(event);
            }
        }

        void swapBuffers() {
            // No-op for Vulkan, kept for API compatibility
        }

        void close() {
            m_shouldClose = true;
            if (m_eventCallback) {
                WindowCloseEvent event;
                dispatchEvent(event);
            }
        }

        bool shouldClose() const { return m_shouldClose; }
        bool isMinimized() const {
            if (!m_window) return false;
            return (SDL_GetWindowFlags(m_window.get()) & SDL_WINDOW_MINIMIZED) != 0;
        }
        bool isFocused() const { return m_focused; }
        bool isFullscreen() const { return m_fullscreen; }

        uint32_t getWidth() const { return m_width; }
        uint32_t getHeight() const { return m_height; }
        std::pair<uint32_t, uint32_t> getSize() const { return {m_width, m_height}; }

        int32_t getPositionX() const {
            if (!m_window) return 0;
            int x, y;
            SDL_GetWindowPosition(m_window.get(), &x, &y);
            return x;
        }

        int32_t getPositionY() const {
            if (!m_window) return 0;
            int x, y;
            SDL_GetWindowPosition(m_window.get(), &x, &y);
            return y;
        }

        std::pair<int32_t, int32_t> getPosition() const {
            if (!m_window) return {0, 0};
            int x, y;
            SDL_GetWindowPosition(m_window.get(), &x, &y);
            return {x, y};
        }

        const std::string& getTitle() const { return m_title; }

        void setSize(uint32_t width, uint32_t height) {
            if (!m_window || width == 0 || height == 0) return;
            SDL_SetWindowSize(m_window.get(), static_cast<int>(width), static_cast<int>(height));
            m_width = width;
            m_height = height;
        }

        void setPosition(int32_t x, int32_t y) {
            if (!m_window) return;
            SDL_SetWindowPosition(m_window.get(), x, y);
        }

        void setTitle(const std::string& title) {
            if (!m_window || title.empty()) return;
            SDL_SetWindowTitle(m_window.get(), title.c_str());
            m_title = title;
        }

        void setFullscreen(bool fullscreen) {
            if (!m_window || m_fullscreen == fullscreen) return;
            if (SDL_SetWindowFullscreen(m_window.get(), fullscreen) == 0) {
                m_fullscreen = fullscreen;
            } else {
                AE_ERROR("Failed to set fullscreen mode: {}", SDL_GetError());
            }
        }

        void setVSync(bool enabled) { m_vsync = enabled; }
        void setEventCallback(const EventCallbackFn& callback) { m_eventCallback = callback; }

        SDL_Window* getNativeWindow() const { return m_window.get(); }

        void* getNativeHandle() const {
            if (!m_window) return nullptr;
            #ifdef _WIN32
                return SDL_GetPointerProperty(SDL_GetWindowProperties(m_window.get()), "SDL.window.win32.hwnd", nullptr);
            #else
                return nullptr; // Other platforms not implemented for this example
            #endif
        }

        float getAspectRatio() const { return static_cast<float>(m_width) / static_cast<float>(m_height); }
        void center() { if (m_window) SDL_SetWindowPosition(m_window.get(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED); }
        void maximize() { if (m_window) SDL_MaximizeWindow(m_window.get()); }
        void minimize() { if (m_window) SDL_MinimizeWindow(m_window.get()); }
        void restore() { if (m_window) SDL_RestoreWindow(m_window.get()); }

        void logWindowInfo() const {
            if (!m_initialized) {
                AE_INFO("Window not initialized");
                return;
            }
            AE_INFO("=== Window Information ===");
            AE_INFO("Title: '{}'", m_title);
            AE_INFO("Size: {}x{}", m_width, m_height);
            // ... etc
            AE_INFO("=========================");
        }

    private:
        // Private methods
        void initSDL() {
            if (s_sdlInitialized) {
                s_windowCount++;
                return;
            }
            if (!SDL_Init(SDL_INIT_VIDEO)) {
                throw std::runtime_error(std::string("Failed to initialize SDL3 video: ") + SDL_GetError());
            }
            s_sdlInitialized = true;
            s_windowCount = 1;
            AE_INFO("SDL3 video subsystem initialized.");
        }

        void shutdownSDL() {
            if (!s_sdlInitialized) return;
            s_windowCount--;
            if (s_windowCount == 0) {
                SDL_Quit();
                s_sdlInitialized = false;
                AE_INFO("SDL3 video subsystem shutdown.");
            }
        }

        void createWindow(const WindowConfig& config) {
            Uint32 flags = SDL_WINDOW_VULKAN;
            if (config.resizable) flags |= SDL_WINDOW_RESIZABLE;
            if (config.fullscreen) flags |= SDL_WINDOW_FULLSCREEN;
            if (!config.decorated) flags |= SDL_WINDOW_BORDERLESS;
            if (config.focused) flags |= SDL_WINDOW_INPUT_FOCUS;

            m_window.reset(SDL_CreateWindow(config.title.c_str(), config.width, config.height, flags));

            if (!m_window) {
                throw std::runtime_error(std::string("Failed to create SDL window: ") + SDL_GetError());
            }

            m_title = config.title;
            m_width = config.width;
            m_height = config.height;
            m_fullscreen = config.fullscreen;
            m_resizable = config.resizable;
            m_decorated = config.decorated;
            m_vsync = config.vsync;
            m_focused = config.focused;
        }

        void processEvent(const SDL_Event& event) {
            switch (event.type) {
                case SDL_EVENT_QUIT: {
                    close();
                    break;
                }
                case SDL_EVENT_WINDOW_RESIZED: {
                    m_width = static_cast<uint32_t>(event.window.data1);
                    m_height = static_cast<uint32_t>(event.window.data2);
                    if (m_eventCallback) {
                        WindowResizeEvent resizeEvent(m_width, m_height);
                        dispatchEvent(resizeEvent);
                    }
                    break;
                }
                // ... other events
            }
        }

        void dispatchEvent(Event& event) {
            if (m_eventCallback) {
                m_eventCallback(event);
            }
        }

        void updateWindowProperties() {
            if (!m_window) return;
            int w, h;
            SDL_GetWindowSize(m_window.get(), &w, &h);
            m_width = w;
            m_height = h;
            // ... etc
        }

        // Member variables
        std::unique_ptr<SDL_Window, SdlWindowDeleter> m_window;
        std::string m_title;
        uint32_t m_width = 0, m_height = 0;
        bool m_fullscreen = false, m_resizable = true, m_decorated = true, m_vsync = true;
        bool m_shouldClose = false, m_focused = true, m_initialized = false;
        EventCallbackFn m_eventCallback;

        static bool s_sdlInitialized;
        static uint32_t s_windowCount;
    };

    // Static member initialization
    bool Window::WindowImpl::s_sdlInitialized = false;
    uint32_t Window::WindowImpl::s_windowCount = 0;

    //=============================================================================
    // Window Public API -> Forwarding to Impl
    //=============================================================================

    Window::Window(const WindowConfig& config) : m_impl(std::make_unique<WindowImpl>(config)) {}
    Window::Window(const std::string& title, uint32_t width, uint32_t height) {
        WindowConfig config;
        config.title = title;
        config.width = width;
        config.height = height;
        m_impl = std::make_unique<WindowImpl>(config);
    }
    Window::~Window() = default;

    void Window::pollEvents() { m_impl->pollEvents(); }
    void Window::swapBuffers() { m_impl->swapBuffers(); }
    void Window::close() { m_impl->close(); }
    bool Window::shouldClose() const { return m_impl->shouldClose(); }
    bool Window::isMinimized() const { return m_impl->isMinimized(); }
    bool Window::isFocused() const { return m_impl->isFocused(); }
    bool Window::isFullscreen() const { return m_impl->isFullscreen(); }
    uint32_t Window::getWidth() const { return m_impl->getWidth(); }
    uint32_t Window::getHeight() const { return m_impl->getHeight(); }
    std::pair<uint32_t, uint32_t> Window::getSize() const { return m_impl->getSize(); }
    int32_t Window::getPositionX() const { return m_impl->getPositionX(); }
    int32_t Window::getPositionY() const { return m_impl->getPositionY(); }
    std::pair<int32_t, int32_t> Window::getPosition() const { return m_impl->getPosition(); }
    const std::string& Window::getTitle() const { return m_impl->getTitle(); }
    void Window::setSize(uint32_t width, uint32_t height) { m_impl->setSize(width, height); }
    void Window::setPosition(int32_t x, int32_t y) { m_impl->setPosition(x, y); }
    void Window::setTitle(const std::string& title) { m_impl->setTitle(title); }
    void Window::setFullscreen(bool fullscreen) { m_impl->setFullscreen(fullscreen); }
    void Window::setVSync(bool enabled) { m_impl->setVSync(enabled); }
    void Window::setEventCallback(const EventCallbackFn& callback) { m_impl->setEventCallback(callback); }
    SDL_Window* Window::getNativeWindow() const { return m_impl->getNativeWindow(); }
    void* Window::getNativeHandle() const { return m_impl->getNativeHandle(); }
    float Window::getAspectRatio() const { return m_impl->getAspectRatio(); }
    void Window::center() { m_impl->center(); }
    void Window::maximize() { m_impl->maximize(); }
    void Window::minimize() { m_impl->minimize(); }
    void Window::restore() { m_impl->restore(); }
    void Window::logWindowInfo() const { m_impl->logWindowInfo(); }

} // namespace AstralEngine