#pragma once

#include <string>
#include <cstdint>

struct SDL_Window;

namespace Astral {
    class Window {
    public:
        Window(const std::string& title, uint32_t width, uint32_t height);
        ~Window();

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        bool ShouldClose() const { return m_shouldClose; }
        void PollEvents();
        
        SDL_Window* GetNativeWindow() const { return m_window; }

    private:
        SDL_Window* m_window = nullptr;
        bool m_shouldClose = false;
    };
}
