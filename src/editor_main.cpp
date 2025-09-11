#include "Core/Logger.h"
#include "Core/AssetManager.h"
#include "Core/AssetLocator.h"
#include "Core/PerformanceMonitor.h"
#include "Core/EngineConfig.h"
#include "Core/MemoryManager.h"
#include "Platform/Window.h"
#include "Renderer/RendererComponents.h"
#include "Events/Events.h"
#include "ECS/ECS.h"
#include "ECS/ArchetypeECS.h"
#include "UI/UIManager.h"
#include "2D/Layer.h"
#include "2D/Brush.h"
#include "2D/Canvas.h"
#include "2D/Tool.h"
#include "Asset/ImageAssetManager.h"
#include <stdexcept>
#include <chrono>
#include <thread>

int main(int argc, char* argv[]) {
    try {
        // Logger'ı başlat (bu ilk çağrı olmalı)
        AstralEngine::Logger::Init();
        
        AE_INFO("Astral Creative Suite Başlatılıyor...");
        AE_DEBUG("Argümanlar: {} adet", argc);
        
        // AssetLocator'ı initialize et (logger'dan sonra, diğerlerinden önce)
        AstralEngine::AssetLocator::getInstance().initialize(argv[0]);
        
        // Critical assets'leri validate et
        if (!AstralEngine::AssetLocator::getInstance().validateCriticalAssets()) {
            AE_ERROR("Critical asset validation failed - cannot continue");
            return -1;
        }
        
        // Memory Manager'ı başlat
        AE_INFO("Memory Manager başlatılıyor...");
        AstralEngine::Memory::MemoryManager::getInstance().initialize(16, 8); // 16MB frame, 8MB stack
        AE_INFO("Memory Manager başarıyla başlatıldı");
        
        // Engine configuration'ı başlat ve uygula
        auto& config = AstralEngine::EngineConfig::getInstance();
        config.applyRuntimeLimits();
        
        // Event system'ini başlat
        AstralEngine::InitializeEventSystem();
        
        // Performance monitoring'i başlat
        if (config.enablePerformanceMonitoring) {
            AstralEngine::PerformanceMonitor::setSlowOperationThreshold(config.slowOperationThresholdMs);
            AE_INFO("Performance monitoring enabled (threshold: {:.1f}ms)", config.slowOperationThresholdMs);
        }
        
        // Hata ayıklama bilgisi
        AE_DEBUG("Debug modu etkin");
        AE_DEBUG("İşletim sistemi: Windows");
        AE_DEBUG("CPU çekirdek sayısı: {}", std::thread::hardware_concurrency());
        AE_DEBUG("Target FPS: {}, Max FPS: {}", config.targetFrameRate, config.maxFrameRate);
        
        // AssetManager'ı başlat
        AE_DEBUG("AssetManager başlatılıyor...");
        AstralEngine::AssetManager::init();
        AE_INFO("AssetManager başarıyla başlatıldı");

        // Window ve Renderer initialization
        try {
            AE_DEBUG("Pencere oluşturuluyor...");
            
            // Create window with modern configuration approach
            AstralEngine::WindowConfig windowConfig = AstralEngine::WindowConfig::fromEngineConfig();
            windowConfig.title = "Astral Creative Suite v1.0";
            windowConfig.width = 1200;
            windowConfig.height = 800;
            
            AstralEngine::Window window(windowConfig);
            AE_INFO("Pencere başarıyla oluşturuldu");
            
            AE_DEBUG("Renderer oluşturuluyor...");
            AstralEngine::Renderer renderer(window);
            AE_INFO("Renderer başarıyla oluşturuldu");
            
            // Initialize UI system
            AstralEngine::UI::UIManager uiManager;
            uiManager.initialize();
            
            // Create a scene for our editor
            AE_DEBUG("Sahne oluşturuluyor...");
            AstralEngine::ECS::Scene scene;
            
            // Create layer system
            AstralEngine::D2::LayerSystem layerSystem(scene);
            
            // Create brush system
            AstralEngine::D2::BrushSystem brushSystem(scene);
            
            // Create canvas system
            AstralEngine::D2::CanvasSystem canvasSystem(scene, renderer);
            
            // Create tool manager
            AstralEngine::D2::ToolManager toolManager(scene, canvasSystem, brushSystem);
            
            // Register tools
            toolManager.registerTool(std::make_unique<AstralEngine::D2::SelectionTool>());
            toolManager.registerTool(std::make_unique<AstralEngine::D2::BrushTool2D>(brushSystem));
            toolManager.registerTool(std::make_unique<AstralEngine::D2::EraserTool>(brushSystem));
            
            // Select default tool
            toolManager.selectTool("Brush");
            
            // Create asset manager
            AstralEngine::Asset::ImageAssetManager assetManager;
            
            // Add a default layer
            auto baseLayer = layerSystem.addLayer("Background");
            
            // Add a canvas
            auto canvasEntity = canvasSystem.createCanvas(1920, 1080);
            
            AE_INFO("Sahne başarıyla oluşturuldu");
            
            // Set up event handling with the new event system
            window.setEventCallback([&renderer, &uiManager, &canvasSystem, &toolManager, canvasEntity](AstralEngine::Event& event) {
                // Pass event to UI manager
                uiManager.handleEvent(event);
                
                AstralEngine::EventDispatcher dispatcher(event);
                
                // Handle window resize events
                dispatcher.dispatch<AstralEngine::WindowResizeEvent>([&renderer](AstralEngine::WindowResizeEvent& e) {
                    AE_DEBUG("Window resized to {}x{}", e.getWidth(), e.getHeight());
                    renderer.onFramebufferResize();
                    return false; // Allow other handlers to process this event
                });
                
                // Handle window close events
                dispatcher.dispatch<AstralEngine::WindowCloseEvent>([](AstralEngine::WindowCloseEvent& e) {
                    AE_DEBUG("Window close requested by event system");
                    return false; // Allow window to close normally
                });
                
                // Handle window focus events
                dispatcher.dispatch<AstralEngine::WindowFocusEvent>([](AstralEngine::WindowFocusEvent& e) {
                    AE_DEBUG("Window focus changed: {}", e.isFocused() ? "gained" : "lost");
                    return false;
                });
                
                // Handle window lost focus events
                dispatcher.dispatch<AstralEngine::WindowLostFocusEvent>([](AstralEngine::WindowLostFocusEvent& e) {
                    AE_DEBUG("Window lost focus");
                    return false;
                });
                
                // Handle window moved events
                dispatcher.dispatch<AstralEngine::WindowMovedEvent>([](AstralEngine::WindowMovedEvent& e) {
                    AE_DEBUG("Window moved to ({}, {})", e.getX(), e.getY());
                    return false;
                });
            });
            
            AE_INFO("Ana döngü başlıyor...");
            uint32_t frameCount = 0;
            uint32_t consecutiveErrors = 0;
            auto startTime = std::chrono::high_resolution_clock::now();
            auto lastPerfStatsTime = startTime;
            
            while (!window.shouldClose()) {
                PERF_TIMER("MainLoop");
                auto frameStart = std::chrono::high_resolution_clock::now();
                
                // Reset frame allocator at the start of each frame
                AstralEngine::Memory::MemoryManager::getInstance().newFrame();
                
                frameCount++;
                
                // Performance statistics göster (config'e göre)
                if (config.printPerformanceStats && frameCount % config.performanceStatsInterval == 0) {
                    auto currentTime = std::chrono::high_resolution_clock::now();
                    float deltaTime = std::chrono::duration<float>(currentTime - lastPerfStatsTime).count();
                    float fps = config.performanceStatsInterval / deltaTime;
                    AE_INFO("Performance: {:.2f} FPS, Frame: {}, Errors: {}", fps, frameCount, consecutiveErrors);
                    
                    if (config.enablePerformanceMonitoring) {
                        AstralEngine::PerformanceMonitor::printStats();
                    }
                    
                    lastPerfStatsTime = currentTime;
                }
                
                {
                    PERF_TIMER("PollEvents");
                    window.pollEvents();
                }
                
                // Render UI
                uiManager.render();
                
                // Render canvas
                canvasSystem.renderCanvas(canvasEntity, layerSystem.getLayerStack());
                
                // Render active tool
                toolManager.render();
                
                // Render frame with error handling
                try {
                    {
                        PERF_TIMER("Renderer::DrawFrame");
                        renderer.drawFrame(scene);
                    }
                    
                    // Reset error counter on successful frame
                    consecutiveErrors = 0;
                    
                } catch (const std::exception& e) {
                    consecutiveErrors++;
                    AE_ERROR("Frame çizilirken hata oluştu ({}. hata): {}", consecutiveErrors, e.what());
                    
                    // Çok fazla ardışık hata varsa çık
                    if (consecutiveErrors >= config.maxConsecutiveRenderErrors) {
                        AE_ERROR("Çok fazla ardışık render hatası ({}), çıkılıyor", consecutiveErrors);
                        break;
                    }
                    
                    // Hata recovery delay'i
                    std::this_thread::sleep_for(std::chrono::milliseconds(config.errorRecoveryDelayMs));
                    continue;
                }
                
                // FPS limiting (sadece gerekiyorsa)
                if (config.enableFrameRateLimiting) {
                    auto frameEnd = std::chrono::high_resolution_clock::now();
                    auto frameDuration = std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd - frameStart);
                    
                    if (frameDuration.count() < config.frameTimeTargetMs) {
                        auto sleepTime = std::chrono::milliseconds(config.frameTimeTargetMs) - frameDuration;
                        std::this_thread::sleep_for(sleepTime);
                    }
                    
                    // Log slow frames
                    if (config.logSlowFrames && frameDuration.count() > (config.frameTimeTargetMs * 2)) {
                        AE_WARN("Slow frame detected: {}ms (target: {}ms)", 
                               frameDuration.count(), config.frameTimeTargetMs);
                    }
                }
            }

            AE_INFO("Ana döngü sona erdi (Toplam frame: {})", frameCount);
            
            // Final performance stats
            if (config.enablePerformanceMonitoring) {
                AE_INFO("Final Performance Statistics:");
                AstralEngine::PerformanceMonitor::printStats();
            }
            
            // Shutdown UI system
            uiManager.shutdown();
            
        } catch (const std::exception& e) {
            AE_ERROR("Window/Renderer initialization hatası: {}", e.what());
            return 1;
        }

        // Cleanup
        try {
            AE_DEBUG("AssetManager kapatılıyor...");
            AstralEngine::AssetManager::shutdown();
            AE_INFO("AssetManager başarıyla kapatıldı");
            
            // Performance monitoring cleanup
            if (config.enablePerformanceMonitoring) {
                AstralEngine::PerformanceMonitor::reset();
            }
            
            // Event system cleanup
            AE_DEBUG("Event system kapatılıyor...");
            AstralEngine::ShutdownEventSystem();
            
            // Memory Manager cleanup
            AE_DEBUG("Memory Manager kapatılıyor...");
            AstralEngine::Memory::MemoryManager::getInstance().shutdown();
            AE_INFO("Memory Manager başarıyla kapatıldı");
            
        } catch (const std::exception& e) {
            AE_ERROR("AssetManager kapatılırken hata oluştu: {}", e.what());
        }

    } catch (const std::exception& e) {
        AE_ERROR("Kritik bir hata oluştu: {}", e.what());
        
        // Debug bilgisi için stack trace açma
        AE_DEBUG("Hata ayıklama için stack trace bilgisi:");
        
        // Logger'ı temizle
        try {
            AstralEngine::Logger::Shutdown();
        } catch (...) {
            // Hata olursa yok say
        }
        
        return 1;
    }

    AstralEngine::Logger::Shutdown();
    AE_INFO("Astral Creative Suite başarıyla kapatıldı.");
    return 0;
}