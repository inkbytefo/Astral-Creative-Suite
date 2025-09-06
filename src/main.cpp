#include "Core/Logger.h"
#include "Platform/Window.h"
#include "Renderer/Renderer.h"
#include <stdexcept>

int main(int argc, char* argv[]) {
    Astral::Logger::Init();
    ASTRAL_LOG_INFO("Astral Engine Başlatılıyor...");

    try {
        Astral::Window window("Astral Engine v1.0", 800, 600);
        Astral::Renderer renderer(window);

        while (!window.ShouldClose()) {
            window.PollEvents();
            renderer.DrawFrame();
        }

    } catch (const std::exception& e) {
        ASTRAL_LOG_ERROR("Kritik bir hata oluştu: {}", e.what());
        return 1;
    }

    ASTRAL_LOG_INFO("Astral Engine Kapatılıyor.");
    return 0;
}
