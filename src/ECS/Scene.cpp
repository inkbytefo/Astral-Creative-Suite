#include "Scene.h"
#include "Core/Logger.h"
#include <algorithm>

namespace AstralEngine::ECS {

    Scene::Scene() {
        AE_INFO("Scene created with default systems");
    }

    Scene::~Scene() {
        AE_INFO("Scene destroyed with {} entities", getEntityCount());
    }

    EntityID Scene::createEntity(const std::string& name) {
        EntityID entity = m_registry.createEntity(name);
        AE_DEBUG("Created entity {} with name '{}'", entity, name);
        return entity;
    }

    void Scene::destroyEntity(EntityID entity) {
        if (entity == m_mainCameraEntity) {
            m_mainCameraEntity = NULL_ENTITY;
        }
        m_registry.destroyEntity(entity);
        AE_DEBUG("Destroyed entity {}", entity);
    }

    bool Scene::isEntityValid(EntityID entity) const {
        return m_registry.isValid(entity);
    }

    void Scene::setEntityName(EntityID entity, const std::string& name) {
        m_registry.setEntityName(entity, name);
    }

    const std::string& Scene::getEntityName(EntityID entity) const {
        return m_registry.getEntityName(entity);
    }

    EntityID Scene::findEntityByName(const std::string& name) const {
        // This is a simple linear search - could be optimized with a map if needed
        for (EntityID entity = 0; entity < static_cast<EntityID>(getEntityCount() + 10); ++entity) {
            if (isEntityValid(entity) && getEntityName(entity) == name) {
                return entity;
            }
        }
        return INVALID_ENTITY;
    }

    void Scene::update(float deltaTime) {
        // Update systems directly
        m_transformSystem.update(m_registry, deltaTime);
    }

    void Scene::setMainCamera(EntityID camera) {
        if (camera != NULL_ENTITY && !hasComponent<Camera>(camera)) {
            AE_WARN("Trying to set main camera to entity {} that doesn't have Camera component", camera);
            return;
        }
        
        m_mainCameraEntity = camera;
        AE_INFO("Main camera set to entity {}", camera);
    }

    size_t Scene::getEntityCount() const {
        return m_registry.getEntityCount();
    }

    size_t Scene::getSystemCount() const {
        return 1; // TransformSystem only (RenderSystem removed)
    }

    void Scene::printDebugInfo() const {
        AE_INFO("=== Scene Debug Info ===");
        AE_INFO("Main Camera Entity: {}", m_mainCameraEntity);
        AE_INFO("Systems Count: {}", getSystemCount());
        m_registry.printDebugInfo();
        AE_INFO("=== End Scene Debug Info ===");
    }

} // namespace AstralEngine::ECS
