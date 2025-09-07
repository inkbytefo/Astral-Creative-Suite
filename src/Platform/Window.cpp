#include "Window.h"
#include "Core/Logger.h"
#include <SDL3/SDL.h>
#include <stdexcept>

namespace Astral {
    Window::Window(const std::string& title, uint32_t width, uint32_t height) {
        // SDL3: SDL_Init() returns bool (true=success, false=failure)
        // NOT an integer like SDL2, so we use direct boolean check
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            throw std::runtime_error(std::string("SDL başlatılamadı: ") + SDL_GetError());
        }

        m_window = SDL_CreateWindow(
            title.c_str(),
            width,
            height,
            SDL_WINDOW_VULKAN
        );

        if (!m_window) {
            SDL_Quit();
            throw std::runtime_error(std::string("Pencere oluşturulamadı: ") + SDL_GetError());
        }
        ASTRAL_LOG_INFO("Pencere başarıyla oluşturuldu.");
    }

    Window::~Window() {
        SDL_DestroyWindow(m_window);
        SDL_Quit();
        ASTRAL_LOG_INFO("Pencere yok edildi ve SDL kapatıldı.");
    }

    void Window::PollEvents() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                m_shouldClose = true;
            }
        }
    }
}
