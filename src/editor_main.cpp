#include "Core/Logger.h"
#include "Core/AssetManager.h"
#include "Core/AssetLocator.h"
#include "Core/PerformanceMonitor.h"
#include "Core/EngineConfig.h"
#include "Core/MemoryManager.h"
#include "Platform/Window.h"
#include "ECS/RenderComponents.h"
#include "Events/Events.h"
#include "ECS/ECS.h"
#include "ECS/ArchetypeECS.h"
#include "UI/UIManager.h"
#include "2D/Layers/Layer.h"
#include "2D/Tools/Brush.h"
#include "2D/Canvas/Canvas.h"
#include "2D/Tools/Tool.h"
#include "Asset/ImageAssetManager.h"
#include "Asset/ModelAsset.h"
#include "Renderer/RRenderer.h"

#include <imgui.h>
#include <stdexcept>
#include <chrono>
#include <thread>




int main(int argc, char* argv[]) {
    try {
        AstralEngine::Logger::Init();
        AE_INFO("Astral Creative Suite Starting...");

        AstralEngine::AssetLocator::getInstance().initialize(argv[0]);
        if (!AstralEngine::AssetLocator::getInstance().validateCriticalAssets()) {
            AE_ERROR("Critical asset validation failed - cannot continue");
            return -1;
        }

        AstralEngine::Memory::MemoryManager::getInstance().initialize(16, 8);
        auto& config = AstralEngine::EngineConfig::getInstance();
        config.applyRuntimeLimits();
        AstralEngine::InitializeEventSystem();

        if (config.enablePerformanceMonitoring) {
            AstralEngine::PerformanceMonitor::setSlowOperationThreshold(config.slowOperationThresholdMs);
        }

        AstralEngine::AssetManager::init();

        try {
            AstralEngine::WindowConfig windowConfig = AstralEngine::WindowConfig::fromEngineConfig();
            windowConfig.title = "Astral Creative Suite v1.0";
            windowConfig.width = 1600;
            windowConfig.height = 900;
            
            AstralEngine::Window window(windowConfig);
            AstralEngine::Renderer renderer(window);

            AstralEngine::UI::UIManager uiManager;
            AstralEngine::ECS::Scene scene;
            
            // 2D Systems
            AstralEngine::D2::LayerSystem layerSystem(scene);
            AstralEngine::D2::BrushSystem brushSystem(scene);
            AstralEngine::D2::CanvasSystem canvasSystem(scene, renderer);
            AstralEngine::D2::ToolManager toolManager(scene, canvasSystem, brushSystem);
            
            uiManager.Initialize(window, renderer, toolManager);

            toolManager.registerTool(std::make_unique<AstralEngine::D2::SelectionTool>());
            toolManager.registerTool(std::make_unique<AstralEngine::D2::BrushTool2D>(brushSystem, layerSystem));
            toolManager.registerTool(std::make_unique<AstralEngine::D2::EraserTool>(brushSystem, layerSystem));
            toolManager.selectTool("Brush");

            auto baseLayer = layerSystem.addLayer("Background");
            auto canvasEntity = canvasSystem.createCanvas(1920, 1080);

            // Test 3D model loading with dependency injection (no global device hack)
            auto modelAsset = std::make_shared<AstralEngine::ModelAsset>("models/viking_room.obj", renderer.GetDevice());
            modelAsset->load();

            if (modelAsset && modelAsset->isLoaded()) {
                auto entity = scene.createEntity("Viking Room");
                scene.addComponent<AstralEngine::ECS::Transform>(entity, glm::vec3(0.0f, 0.0f, 0.0f));
                scene.addComponent<AstralEngine::ECS::RenderComponent>(entity, modelAsset);
                AE_INFO("Test model 'viking_room.obj' loaded and added to scene.");
            } else {
                AE_ERROR("Failed to load test model 'viking_room.obj'.");
            }

            AE_INFO("Main loop starting...");
            while (!window.shouldClose()) {
                AstralEngine::Memory::MemoryManager::getInstance().newFrame();
                window.pollEvents();
                
                uiManager.BeginFrame();
                uiManager.Render();

                if (uiManager.GetAppState() == AstralEngine::UI::AppState::Editor2D) {
                    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
                    ImGui::Begin("Canvas");
                    {
                        ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
                        if (ImGui::IsWindowHovered() && toolManager.getActiveTool()) {
                            ImGuiIO& io = ImGui::GetIO();
                            glm::vec2 mouse_pos = {io.MousePos.x - canvas_pos.x, io.MousePos.y - canvas_pos.y};
                            if (ImGui::IsMouseDown(0)) {
                                if (ImGui::IsMouseClicked(0)) { toolManager.getActiveTool()->onMouseDown(mouse_pos, canvasEntity); }
                                toolManager.getActiveTool()->onMouseMove(mouse_pos, canvasEntity);
                            } else if (ImGui::IsMouseReleased(0)) { 
                                toolManager.getActiveTool()->onMouseUp(mouse_pos, canvasEntity);
                            }
                        }
                    }
                    ImGui::End();
                    ImGui::PopStyleVar();

                    // Note: canvasSystem.renderCanvas should be moved to CanvasPass
                    // For now, keep it here but it should be integrated into Vulkan pipeline
                    canvasSystem.renderCanvas(canvasEntity, layerSystem.getLayerStack());
                    toolManager.render();
                }
                
                // Render scene with ImGui integration - pass ImGui data to RenderGraph
                renderer.DrawScene(scene, ImGui::GetDrawData());
            }

            uiManager.Shutdown();
            
        } catch (const std::exception& e) {
            AE_ERROR("Window/Renderer initialization error: {}", e.what());
            return 1;
        }

        AstralEngine::AssetManager::shutdown();
        AstralEngine::ShutdownEventSystem();
        AstralEngine::Memory::MemoryManager::getInstance().shutdown();

    } catch (const std::exception& e) {
        AE_ERROR("A critical error occurred: {}", e.what());
        AstralEngine::Logger::Shutdown();
        return 1;
    }

    AstralEngine::Logger::Shutdown();
    AE_INFO("Astral Creative Suite has been shut down successfully.");
    return 0;
}
