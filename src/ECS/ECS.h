#pragma once

// Main ECS header - includes all ECS functionality
// This is the primary header that should be included by users of the ECS system
// NOW POWERED BY HIGH-PERFORMANCE ARCHETYPE SYSTEM!

#include "ArchetypeECS.h"
#include "Components.h"
#include "RenderComponents.h"
#include "Scene.h"

namespace AstralEngine::ECS {
    // Convenience aliases for commonly used types
    using Entity = EntityID;
    
    // Convenience functions for component type registration
    template<typename T>
    inline ComponentTypeID GetComponentType() {
        return ComponentTypeRegistry::getTypeID<T>();
    }
    
    template<typename T>
    inline const char* GetComponentTypeName() {
        return ComponentTypeRegistry::getTypeName<T>();
    }
}

// Legacy namespace compatibility (for gradual migration)
namespace AstralEngine {
    // Create aliases in the main namespace for backward compatibility
    using Scene = ECS::Scene;
    using EntityID = ECS::EntityID;
    
    // Legacy Transform type (still available but should use ECS::Transform)
    using Transform = ECS::Transform;
}
