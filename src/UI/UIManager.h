#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include "Core/Logger.h"
#include "Events/Events.h"
#include "Renderer/VulkanR/Vulkan.h"
#include <imgui.h>

// Forward declare ImGui types
struct ImGuiContext;

// Forward declare platform types
struct SDL_Window;

namespace AstralEngine {
    // Forward declare engine types
    class Window;
    namespace D2 {
        class ToolManager;
    }
    class Renderer;

    namespace UI {
        
        // Represents the current high-level state of the application UI
        enum class AppState {
            WelcomeScreen,
            Editor2D,
            Editor3D
        };

        class UIWindow;
        
        class UIManager {
        public:
            UIManager();
            ~UIManager();
            
            UIManager(const UIManager&) = delete;
            UIManager& operator=(const UIManager&) = delete;
            
            void Initialize(AstralEngine::Window& window, Renderer& renderer, D2::ToolManager& toolManager);
            void Shutdown();
            
            void BeginFrame();
            void Render();
            void EndFrame();

            void HandleEvent(Event& event);

            // State management
            void SetAppState(AppState state);
            AppState GetAppState() const;

            // ImGui draw data access for renderer integration
            ImDrawData* GetCurrentDrawData() const { return m_currentDrawData; }

        private:
            void DrawWelcomeScreen();
            void Draw2DEditor();
            void Draw3DEditorPlaceholder();

            void SetupDockspace();
            void DrawToolbar();
            void DrawPropertiesPanel();

            bool m_initialized = false;
            ImGuiContext* m_context = nullptr;
            VkDescriptorPool m_imguiPool = VK_NULL_HANDLE;

            AppState m_appState = AppState::WelcomeScreen; // Start with the welcome screen

            // ImGui draw data for renderer integration
            ImDrawData* m_currentDrawData = nullptr;

            // Keep raw pointers to major systems
            AstralEngine::Window* m_window = nullptr;
            Renderer* m_renderer = nullptr;
            D2::ToolManager* m_toolManager = nullptr;
        };
    }
}
