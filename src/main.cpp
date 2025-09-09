#include "Core/Logger.h"
#include "Core/AssetManager.h"
#include "Core/PerformanceMonitor.h"
#include "Core/EngineConfig.h"
#include "Core/MemoryManager.h"
#include "Platform/Window.h"
#include "Renderer/Renderer.h"
#include "Renderer/Model.h"
#include "Renderer/UnifiedMaterial.h"
#include "ECS/ECS.h"
#include "ECS/ArchetypeECS.h"
#include <stdexcept>
#include <chrono>
#include <thread>

int main(int argc, char* argv[]) {
    try {
        AE_INFO("Astral Engine Başlatılıyor...");
        AE_DEBUG("Argümanlar: {} adet", argc);
        
        // Logger'ı başlat
        AstralEngine::Logger::Init();
        
        // Memory Manager'ı başlat
        AE_INFO("Memory Manager başlatılıyor...");
        AstralEngine::Memory::MemoryManager::getInstance().initialize(16, 8); // 16MB frame, 8MB stack
        AE_INFO("Memory Manager başarıyla başlatıldı");
        
        // Engine configuration'ı başlat ve uygula
        auto& config = AstralEngine::EngineConfig::getInstance();
        config.applyRuntimeLimits();
        
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
            AstralEngine::Window window("Astral Engine v1.0", 800, 600);
            AE_INFO("Pencere başarıyla oluşturuldu");
            
            AE_DEBUG("Renderer oluşturuluyor...");
            AstralEngine::Renderer renderer(window);
            AE_INFO("Renderer başarıyla oluşturuldu");
            
            // Create a scene
            AE_DEBUG("Sahne oluşturuluyor...");
            AstralEngine::ECS::Scene scene;
            AE_INFO("Sahne başarıyla oluşturuldu");
            
            // Create a camera entity
            AE_DEBUG("Kamera oluşturuluyor...");
            auto cameraEntity = scene.createEntity("Main Camera");
            auto& cameraTransform = scene.addComponent<AstralEngine::ECS::Transform>(cameraEntity);
            cameraTransform.position = glm::vec3(0.0f, 0.0f, 3.0f);
            
            auto& camera = scene.addComponent<AstralEngine::ECS::Camera>(cameraEntity, 45.0f, 800.0f / 600.0f, 0.1f, 100.0f);
            scene.setMainCamera(cameraEntity);
            AE_INFO("Kamera başarıyla oluşturuldu ve ana kamera olarak atandı");
            
            // Create an entity with a model
            AE_DEBUG("Entity oluşturuluyor...");
            auto entityId = scene.createEntity("Cube");
            auto& transform = scene.addComponent<AstralEngine::ECS::Transform>(entityId);
            transform.position = glm::vec3(0.0f, 0.0f, 0.0f);
            transform.scale = glm::vec3(0.5f);
            AE_INFO("Entity başarıyla oluşturuldu");
            
            // Try to load a model, fallback to a default one if not found
            std::shared_ptr<AstralEngine::Model> model = nullptr;
            try {
                AE_DEBUG("Model yükleniyor: assets/models/cube.obj");
                auto modelFuture = AstralEngine::AssetManager::loadModelAsync(renderer.getDevice(), "assets/models/cube.obj");
                model = modelFuture.get();
                AE_INFO("Model başarıyla yüklendi");
            } catch (const std::exception& e) {
                AE_WARN("Model yüklenemedi: {}", e.what());
                AE_WARN("Sorun続ける: devam edilecek");
            }

            // If model loading failed, continue without it
            if (model) {
                AE_DEBUG("Entity'ye render bileşeni atanıyor...");
                
                // Create a basic material instance
                auto material = std::make_shared<AstralEngine::UnifiedMaterialInstance>();
                *material = AstralEngine::UnifiedMaterialInstance::createDefault();
                
                auto& renderComponent = scene.addComponent<AstralEngine::ECS::RenderComponent>(entityId);
                renderComponent.model = model;
                renderComponent.materialOverride = material;
                AE_INFO("Render bileşeni başarıyla atanıldı");

                // İkinci bir entity oluşturup deneyin!
                auto entity2Id = scene.createEntity("Cube 2");
                auto& transform2 = scene.addComponent<AstralEngine::ECS::Transform>(entity2Id);
                transform2.position = glm::vec3(2.0f, 0.0f, 0.0f);
                
                // Create another material instance (with different properties)
                auto material2 = std::make_shared<AstralEngine::UnifiedMaterialInstance>();
                *material2 = AstralEngine::UnifiedMaterialInstance::createDefault();
                material2->setBaseColor(glm::vec4(1.0f, 0.5f, 0.5f, 1.0f)); // Red tint
                
                auto& renderComponent2 = scene.addComponent<AstralEngine::ECS::RenderComponent>(entity2Id);
                renderComponent2.model = model;
                renderComponent2.materialOverride = material2;
                AE_INFO("İkinci entity başarıyla oluşturuldu");
            } else {
                AE_WARN("Model olmadan devam ediliyor");
                AE_DEBUG("Render component atanmadan devam ediliyor");
            }

            AE_INFO("Ana döngü başlıyor...");
            uint32_t frameCount = 0;
            uint32_t consecutiveErrors = 0;
            auto startTime = std::chrono::high_resolution_clock::now();
            auto lastPerfStatsTime = startTime;
            
            while (!window.ShouldClose()) {
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
                
                // Check if renderer wants to exit
                if (!renderer.canRender()) {
                    AE_WARN("Renderer cannot render, checking exit condition");
                    // If renderer is in bad state, give it some time to recover
                    std::this_thread::sleep_for(std::chrono::milliseconds(config.errorRecoveryDelayMs));
                    continue;
                }
                
                {
                    PERF_TIMER("PollEvents");
                    window.PollEvents();
                }
                
                // Entity'yi her frame döndürelim
                if (scene.hasComponent<AstralEngine::ECS::Transform>(entityId)) {
                    auto& transform = scene.getComponent<AstralEngine::ECS::Transform>(entityId);
                    transform.rotation.z += 0.01f;
                    transform.markDirty();
                }
                
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
    AE_INFO("Astral Engine başarıyla kapatıldı.");
    return 0;
}
