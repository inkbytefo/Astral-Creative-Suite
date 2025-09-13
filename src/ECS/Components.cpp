#include "Components.h"
#include "ArchetypeECS.h"
#include "Renderer/Model.h"
#include "Renderer/UnifiedMaterial.h"
#include "Asset/ModelAsset.h" // Add full definition for ModelAsset

namespace AstralEngine::ECS {

    glm::mat4 Transform::getWorldMatrix(ArchetypeRegistry& registry) const {
        if (!dirty && parent == NULL_ENTITY) {
            return worldMatrix;
        }
        
        glm::mat4 local = getLocalMatrix();
        
        if (parent != NULL_ENTITY && registry.hasComponent<Transform>(parent)) {
            const auto& parentTransform = registry.getComponent<Transform>(parent);
            worldMatrix = parentTransform.getWorldMatrix(registry) * local;
        } else {
            worldMatrix = local;
        }
        
        dirty = false;
        return worldMatrix;
    }
    
        std::shared_ptr<UnifiedMaterialInstance> RenderComponent::getEffectiveMaterial(uint32_t submeshIndex) const {
        if (materialOverride) {
            return materialOverride;
        }
        if (modelAsset && modelAsset->isLoaded()) {
            auto model = modelAsset->getModel();
            if (model) {
                return model->getSubmeshMaterial(submeshIndex);
            }
        }
        return nullptr;
    }


} // namespace AstralEngine::ECS
