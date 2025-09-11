#pragma once

#include "ArchetypeECS.h"
#include "RenderComponents.h"
#include <string>
#include <memory>

namespace AstralEngine::ECS {

    class Scene {
    public:
        Scene();
        ~Scene();

        // Entity management
        EntityID createEntity(const std::string& name = "Entity");
        void destroyEntity(EntityID entity);
        bool isEntityValid(EntityID entity) const;

        // Entity naming
        void setEntityName(EntityID entity, const std::string& name);
        const std::string& getEntityName(EntityID entity) const;
        EntityID findEntityByName(const std::string& name) const;

        // Component management (forwarded to archetype registry)
        template<typename T, typename... Args>
        T& addComponent(EntityID entity, Args&&... args) {
            return m_registry.addComponent<T>(entity, std::forward<Args>(args)...);
        }

        template<typename T>
        void removeComponent(EntityID entity) {
            m_registry.removeComponent<T>(entity);
        }

        template<typename T>
        bool hasComponent(EntityID entity) const {
            return m_registry.hasComponent<T>(entity);
        }

        template<typename T>
        T& getComponent(EntityID entity) {
            return m_registry.getComponent<T>(entity);
        }

        template<typename T>
        const T& getComponent(EntityID entity) const {
            return const_cast<ArchetypeRegistry&>(m_registry).getComponent<T>(entity);
        }

        // View creation (forwarded to archetype registry)
        template<typename... Components>
        auto view() {
            return m_registry.view<Components...>();
        }

        template<typename... Components>
        auto view() const {
            return const_cast<ArchetypeRegistry&>(m_registry).view<Components...>();
        }

        // Scene update (simple system execution)
        void update(float deltaTime);
        
        // System access
        TransformSystem& getTransformSystem() { return m_transformSystem; }
        RenderSystem& getRenderSystem() { return m_renderSystem; }

        // Camera management
        void setMainCamera(EntityID camera);
        EntityID getMainCamera() const { return m_mainCameraEntity; }
        bool hasMainCamera() const { return m_mainCameraEntity != INVALID_ENTITY; }

        // Scene information
        size_t getEntityCount() const;
        size_t getSystemCount() const;
        void printDebugInfo() const;

        // Direct registry access (for advanced use cases)
        ArchetypeRegistry& getRegistry() { return m_registry; }
        const ArchetypeRegistry& getRegistry() const { return m_registry; }

        // Scene serialization (future implementation)
        // void saveToFile(const std::string& filename);
        // void loadFromFile(const std::string& filename);

    private:
        ArchetypeRegistry m_registry;
        TransformSystem m_transformSystem;
        RenderSystem m_renderSystem;
        EntityID m_mainCameraEntity = INVALID_ENTITY;
    };

} // namespace AstralEngine::ECS
