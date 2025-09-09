#pragma once

#include "Components.h"
#include <algorithm>

// Forward declarations
namespace AstralEngine {
    class UnifiedMaterialInstance;
}

namespace AstralEngine::ECS {

    // System for updating transform hierarchies
    class TransformSystem {
    public:
        void update(ArchetypeRegistry& registry, float deltaTime) {
            // Process root transforms first, then propagate to children
            auto view = registry.view<Transform>();
            
            for (auto [entity, transform] : view) {
                if (transform.parent == INVALID_ENTITY && transform.dirty) {
                    updateTransformHierarchy(registry, entity, transform);
                }
            }
        }
        
        const char* getName() const { return "TransformSystem"; }
        int getPriority() const { return -100; } // Run early
        
    private:
        void updateTransformHierarchy(ArchetypeRegistry& registry, EntityID entity, Transform& transform) {
            // Update world matrix
            transform.getWorldMatrix(registry);
            
            // Update children
            for (EntityID child : transform.children) {
                if (registry.hasComponent<Transform>(child)) {
                    auto& childTransform = registry.getComponent<Transform>(child);
                    childTransform.markDirty();
                    updateTransformHierarchy(registry, child, childTransform);
                }
            }
        }
    };

    // System for rendering entities with RenderComponent
    class RenderSystem {
    public:
        void update(ArchetypeRegistry& registry, float deltaTime) {
            // Get active camera
            Camera* activeCamera = nullptr;
            Transform* cameraTransform = nullptr;
            
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
            
            // Collect all renderable entities
            std::vector<std::pair<EntityID, float>> renderQueue;
            
            auto renderView = registry.view<Transform, RenderComponent>();
            glm::vec3 cameraPos = cameraTransform->position;
            
            for (auto [entity, transform, renderComp] : renderView) {
                if (!renderComp.visible || !renderComp.model || !renderComp.materialOverride) {
                    continue;
                }
                
                // Calculate distance for sorting
                float distance = glm::length(transform.position - cameraPos);
                
                // Frustum culling could be added here
                
                renderQueue.emplace_back(entity, distance);
            }
            
            // Sort by distance (back to front for transparency, front to back for opaque)
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
                
                // For now, just mark as rendered (placeholder)
                // renderEntity(transform.getWorldMatrix(registry), renderComp);
            }
        }
        
        const char* getName() const { return "RenderSystem"; }
        int getPriority() const { return 100; } // Run late
    };

} // namespace AstralEngine::ECS
