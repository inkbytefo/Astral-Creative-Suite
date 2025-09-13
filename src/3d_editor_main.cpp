#include "Core/Logger.h"
#include "Core/AssetManager.h"
#include "Core/AssetLocator.h"
#include "Platform/Window.h"
#include "ECS/Scene.h"
#include "Events/Events.h"
#include "UI/UIManager.h"
#include "Renderer/RRenderer.h"  // Our new refactored renderer
#include "3D/Components3D.h"
#include "3D/Render3DSystem.h"
#include "Asset/MeshAsset.h"
#include "Asset/MaterialAsset.h"

#include <imgui.h>
#include <stdexcept>
#include <chrono>

int main(int argc, char* argv[]) {
    try {
        AstralEngine::Logger::Init();
        AE_INFO("Astral Creative Suite 3D Editor Starting...");

        AstralEngine::AssetLocator::getInstance().initialize(argv[0]);
        if (!AstralEngine::AssetLocator::getInstance().validateCriticalAssets()) {
            AE_ERROR("Critical asset validation failed - cannot continue");
            return -1;
        }

        AstralEngine::InitializeEventSystem();

        try {
            AstralEngine::WindowConfig windowConfig;
            windowConfig.title = "Astral Creative Suite 3D Editor";
            windowConfig.width = 1600;
            windowConfig.height = 900;
            
            AstralEngine::Window window(windowConfig);
            AstralEngine::Renderer renderer(window);

            AstralEngine::UI::UIManager uiManager;
            AstralEngine::ECS::Scene scene;
            
            // Create the 3D render system
            AstralEngine::D3::Render3DSystem render3DSystem(renderer.getDevice());
            
            uiManager.Initialize(window, renderer);

            // Create a simple 3D scene with a camera and a mesh
            auto cameraEntity = scene.createEntity("MainCamera");
            scene.addComponent<AstralEngine::D3::Transform3D>(cameraEntity, 
                glm::vec3(0.0f, 0.0f, 5.0f),   // Position
                glm::vec3(0.0f, 0.0f, 0.0f),   // Rotation
                glm::vec3(1.0f));              // Scale
            scene.addComponent<AstralEngine::D3::Camera3D>(cameraEntity, 45.0f, 16.0f/9.0f, 0.1f, 1000.0f);

            // Create a simple cube mesh entity
            auto cubeEntity = scene.createEntity("Cube");
            scene.addComponent<AstralEngine::D3::Transform3D>(cubeEntity,
                glm::vec3(0.0f, 0.0f, 0.0f),   // Position
                glm::vec3(0.0f, 0.0f, 0.0f),   // Rotation
                glm::vec3(1.0f));              // Scale
            // For now, we'll just add the component without an actual mesh asset
            // In a full implementation, we would load a mesh asset here
            // Create a mesh asset and add it to the mesh component
            // auto meshAsset = std::make_shared<AstralEngine::MeshAsset>("cube.obj", renderer.getDevice());
            // meshAsset->load();
            // scene.addComponent<AstralEngine::D3::Mesh3D>(cubeEntity, meshAsset);
            scene.addComponent<AstralEngine::D3::Mesh3D>(cubeEntity);

            AE_INFO("3D Editor main loop starting...");
            while (!window.shouldClose()) {
                window.pollEvents();
                
                // Update UI
                uiManager.BeginFrame();
                
                // Create a simple UI panel to show that the editor is running
                ImGui::Begin("3D Editor");
                ImGui::Text("Astral Creative Suite 3D Editor");
                ImGui::Text("Frame rate: %.1f FPS", ImGui::GetIO().Framerate);
                ImGui::End();
                
                uiManager.Render();

                // Render the 3D scene
                renderer.DrawScene(scene, ImGui::GetDrawData());
            }

            uiManager.Shutdown();
            
        } catch (const std::exception& e) {
            AE_ERROR("Window/Renderer initialization error: {}", e.what());
            return 1;
        }

        AstralEngine::ShutdownEventSystem();

    } catch (const std::exception& e) {
        AE_ERROR("A critical error occurred: {}", e.what());
        AstralEngine::Logger::Shutdown();
        return 1;
    }

    AstralEngine::Logger::Shutdown();
    AE_INFO("Astral Creative Suite 3D Editor has been shut down successfully.");
    return 0;
}