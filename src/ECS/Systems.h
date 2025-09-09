#pragma once

#include "ArchetypeECS.h"
#include "Components.h"
#include <vector>
#include <memory>
#include <algorithm>
#include <string>

namespace AstralEngine::ECS {

    // System base class for game logic
    class System {
    public:
        virtual ~System() = default;
        virtual void update(ArchetypeRegistry& registry, float deltaTime) = 0;
        virtual const char* getName() const = 0;
        
        // System priority for execution order (lower = earlier)
        virtual int getPriority() const { return 0; }
        
        // Whether system should run this frame
        virtual bool shouldRun(float deltaTime) const { return true; }
        
        // Whether system needs to run every frame
        virtual bool isFrameSystem() const { return true; }
        
        // System initialization (called once)
        virtual void initialize(ArchetypeRegistry& registry) {}
        
        // System cleanup (called when system is removed)
        virtual void shutdown(ArchetypeRegistry& registry) {}
    };

    // System scheduler for managing system execution
    class SystemScheduler {
    public:
        SystemScheduler() = default;
        ~SystemScheduler() = default;
        
        void registerSystem(std::unique_ptr<System> system) {
            system->initialize(*m_registry);
            m_systems.push_back(std::move(system));
            sortSystems();
        }
        
        template<typename T, typename... Args>
        void registerSystem(Args&&... args) {
            registerSystem(std::make_unique<T>(std::forward<Args>(args)...));
        }
        
        void unregisterSystem(const char* systemName) {
            auto it = std::find_if(m_systems.begin(), m_systems.end(),
                [systemName](const std::unique_ptr<System>& system) {
                    return std::string(system->getName()) == systemName;
                });
            
            if (it != m_systems.end()) {
                (*it)->shutdown(*m_registry);
                m_systems.erase(it);
            }
        }
        
        void setRegistry(ArchetypeRegistry* registry) {
            m_registry = registry;
        }
        
        void update(float deltaTime) {
            if (!m_registry) return;
            
            for (auto& system : m_systems) {
                if (system->shouldRun(deltaTime)) {
                    system->update(*m_registry, deltaTime);
                }
            }
        }
        
        size_t getSystemCount() const { return m_systems.size(); }
        
        System* getSystem(const char* systemName) const {
            auto it = std::find_if(m_systems.begin(), m_systems.end(),
                [systemName](const std::unique_ptr<System>& system) {
                    return std::string(system->getName()) == systemName;
                });
            return it != m_systems.end() ? it->get() : nullptr;
        }

    private:
        void sortSystems() {
            std::sort(m_systems.begin(), m_systems.end(),
                [](const std::unique_ptr<System>& a, const std::unique_ptr<System>& b) {
                    return a->getPriority() < b->getPriority();
                });
        }
        
        std::vector<std::unique_ptr<System>> m_systems;
        ArchetypeRegistry* m_registry = nullptr;
    };

    // System for updating transform hierarchies
    class TransformSystem : public System {
    public:
        void update(ArchetypeRegistry& registry, float deltaTime) override {
            // Process root transforms first using high-performance ArchetypeView
            auto view = registry.view<Transform>();
            
            // First pass: find and update root transforms
            for (auto [entity, transform] : view) {
                if (transform.parent == NULL_ENTITY && transform.dirty) {
                    updateTransformHierarchy(registry, entity, transform);
                }
            }
        }
        
        const char* getName() const override { return "TransformSystem"; }
        int getPriority() const override { return -100; } // Run early
        
    private:
        void updateTransformHierarchy(ArchetypeRegistry& registry, EntityID entity, Transform& transform) {
            // Update world matrix - much faster with archetype system
            transform.getWorldMatrix(registry);
            
            // Update children recursively
            for (EntityID child : transform.children) {
                if (registry.hasComponent<Transform>(child)) {
                    auto& childTransform = registry.getComponent<Transform>(child);
                    childTransform.markDirty();
                    updateTransformHierarchy(registry, child, childTransform);
                }
            }
        }
    };

    // System for rendering entities with RenderComponent - MASSIVELY OPTIMIZED with ArchetypeView!
    class RenderSystem : public System {
    public:
        void update(ArchetypeRegistry& registry, float deltaTime) override {
            // Get active camera using high-performance ArchetypeView
            Camera* activeCamera = nullptr;
            Transform* cameraTransform = nullptr;
            
            // ArchetypeView provides cache-friendly iteration over entities with both Transform AND Camera
            auto cameraView = registry.view<Transform, Camera>();
            for (auto [entity, transform, camera] : cameraView) {
                // Use first camera found (in a real system, you'd have camera management)
                activeCamera = &camera;
                cameraTransform = &transform;
                break;
            }
            
            if (!activeCamera || !cameraTransform) {
                return; // No camera to render from
            }
            
            // Collect all renderable entities - HUGE performance improvement with ArchetypeView!
            std::vector<std::pair<EntityID, float>> renderQueue;
            
            // ArchetypeView provides optimal memory layout iteration - components are stored contiguously
            auto renderView = registry.view<Transform, RenderComponent>();
            glm::vec3 cameraPos = cameraTransform->position;
            
            // This loop is now MUCH faster thanks to archetype storage!
            for (auto [entity, transform, renderComp] : renderView) {
                if (!renderComp.visible || !renderComp.model || !renderComp.materialOverride) {
                    continue;
                }
                
                // Calculate distance for sorting
                float distance = glm::length(transform.position - cameraPos);
                
                // Basic distance culling
                if (distance > renderComp.maxDistance) {
                    continue;
                }
                
                // Frustum culling could be added here
                
                renderQueue.emplace_back(entity, distance);
            }
            
            // Sort by distance (front to back for opaque objects)
            std::sort(renderQueue.begin(), renderQueue.end(),
                [](const auto& a, const auto& b) { return a.second < b.second; });
            
            // Render entities (this would interface with your actual renderer)
            for (const auto& [entity, distance] : renderQueue) {
                auto& transform = registry.getComponent<Transform>(entity);
                auto& renderComp = registry.getComponent<RenderComponent>(entity);
                
                // Here you would:
                // 1. Bind the material/pipeline
                // 2. Update uniform buffers with transform.getWorldMatrix(registry)
                // 3. Bind vertex/index buffers from renderComp.model
                // 4. Issue draw call
                
                // For now, just mark as processed (placeholder)
                // renderEntity(transform.getWorldMatrix(registry), renderComp);
            }
        }
        
        const char* getName() const override { return "RenderSystem"; }
        int getPriority() const override { return 100; } // Run late
    };

    // System for basic physics/movement (example)
    class MovementSystem : public System {
    public:
        void update(ArchetypeRegistry& registry, float deltaTime) override {
            // This is just an example - you could add velocity components
            // and move entities based on their velocity using ArchetypeView
            // auto view = registry.view<Transform, Velocity>();
            // for (auto [entity, transform, velocity] : view) {
            //     transform.position += velocity.linear * deltaTime;
            //     transform.rotation += velocity.angular * deltaTime;
            //     transform.markDirty();
            // }
        }
        
        const char* getName() const override { return "MovementSystem"; }
        int getPriority() const override { return 0; } // Middle priority
    };

} // namespace AstralEngine::ECS
