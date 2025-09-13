#pragma once

#include "Components.h"
#include "Asset/ModelAsset.h"

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

} // namespace AstralEngine::ECS
