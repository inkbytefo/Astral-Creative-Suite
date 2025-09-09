#pragma once

#include <vector>
#include <unordered_map>
#include <bitset>
#include <typeindex>
#include <memory>
#include <functional>
#include <algorithm>
#include <cstdint>
#include <tuple>
#include <string>

namespace AstralEngine::ECS {

    // Core ECS type definitions
    using EntityID = uint32_t;
    using ComponentTypeID = uint32_t;
    using ArchetypeID = uint64_t;

    constexpr EntityID NULL_ENTITY = 0;
    constexpr EntityID INVALID_ENTITY = 0; // Alias for compatibility
    constexpr size_t MAX_COMPONENTS = 64;

    // Component signature using bitset
    using ComponentSignature = std::bitset<MAX_COMPONENTS>;

    // Base component interface
    class IComponent {
    public:
        virtual ~IComponent() = default;
    };

    // Component type registry for efficient type handling
    class ComponentTypeRegistry {
    public:
        template<typename T>
        static ComponentTypeID getTypeID() {
            static ComponentTypeID id = generateTypeID<T>();
            return id;
        }

        template<typename T>
        static const char* getTypeName() {
            return typeid(T).name();
        }

        static size_t getRegisteredComponentCount() {
            return s_componentCount;
        }

    private:
        static ComponentTypeID s_componentCount;

        template<typename T>
        static ComponentTypeID generateTypeID() {
            return ++s_componentCount;
        }
    };

    // Forward declarations
    class ArchetypeRegistry;
    class Archetype;
    template<typename T> class ComponentArray;

    // Base class for component arrays (type erasure)
    class IComponentArray {
    public:
        virtual ~IComponentArray() = default;
        virtual void removeEntity(size_t index) = 0;
        virtual void moveEntity(size_t from, size_t to) = 0;
        virtual size_t getSize() const = 0;
        virtual const char* getTypeName() const = 0;
    };

    // Typed component array for cache-friendly storage
    template<typename T>
    class ComponentArray : public IComponentArray {
    public:
        ComponentArray() = default;
        ~ComponentArray() = default;

        // Add component (returns index)
        template<typename... Args>
        size_t emplace(Args&&... args) {
            m_components.emplace_back(std::forward<Args>(args)...);
            return m_components.size() - 1;
        }

        // Get component by index
        T& at(size_t index) { return m_components[index]; }
        const T& at(size_t index) const { return m_components[index]; }

        // Remove entity at index (swap with last)
        void removeEntity(size_t index) override {
            if (index < m_components.size() - 1) {
                m_components[index] = std::move(m_components.back());
            }
            m_components.pop_back();
        }

        // Move entity from one index to another
        void moveEntity(size_t from, size_t to) override {
            if (from != to && from < m_components.size() && to < m_components.size()) {
                m_components[to] = std::move(m_components[from]);
            }
        }

        // Array access
        T* data() { return m_components.data(); }
        const T* data() const { return m_components.data(); }
        size_t getSize() const override { return m_components.size(); }
        
        const char* getTypeName() const override {
            return ComponentTypeRegistry::getTypeName<T>();
        }

        // Iterators
        typename std::vector<T>::iterator begin() { return m_components.begin(); }
        typename std::vector<T>::iterator end() { return m_components.end(); }
        typename std::vector<T>::const_iterator begin() const { return m_components.begin(); }
        typename std::vector<T>::const_iterator end() const { return m_components.end(); }

    private:
        std::vector<T> m_components;
    };

    // Archetype represents entities with same component signature
    class Archetype {
    public:
        explicit Archetype(const ComponentSignature& signature);
        ~Archetype() = default;

        // Entity management
        size_t addEntity(EntityID entity);
        void removeEntity(size_t index);
        EntityID getEntity(size_t index) const;
        size_t findEntity(EntityID entity) const;

        // Component management
        template<typename T>
        void addComponentArray() {
            ComponentTypeID typeID = ComponentTypeRegistry::getTypeID<T>();
            if (m_componentArrays.find(typeID) == m_componentArrays.end()) {
                m_componentArrays[typeID] = std::make_unique<ComponentArray<T>>();
            }
        }

        template<typename T>
        ComponentArray<T>* getComponentArray() {
            ComponentTypeID typeID = ComponentTypeRegistry::getTypeID<T>();
            auto it = m_componentArrays.find(typeID);
            if (it != m_componentArrays.end()) {
                return static_cast<ComponentArray<T>*>(it->second.get());
            }
            return nullptr;
        }

        template<typename T, typename... Args>
        T& addComponent(size_t entityIndex, Args&&... args) {
            auto* array = getComponentArray<T>();
            if (!array) {
                addComponentArray<T>();
                array = getComponentArray<T>();
            }
            
            // Ensure component array grows with entity array
            while (array->getSize() <= entityIndex) {
                array->emplace(); // Default construct
            }
            
            // Replace with actual component
            array->at(entityIndex) = T(std::forward<Args>(args)...);
            return array->at(entityIndex);
        }

        template<typename T>
        T& getComponent(size_t entityIndex) {
            auto* array = getComponentArray<T>();
            return array->at(entityIndex);
        }

        template<typename T>
        const T& getComponent(size_t entityIndex) const {
            auto* array = getComponentArray<T>();
            return array->at(entityIndex);
        }

        // Archetype properties
        const ComponentSignature& getSignature() const { return m_signature; }
        ArchetypeID getID() const { return m_id; }
        size_t getEntityCount() const { return m_entities.size(); }
        
        // Check if archetype contains all specified components
        template<typename... Components>
        bool hasComponents() const {
            ComponentSignature required;
            ((required[ComponentTypeRegistry::getTypeID<Components>() - 1] = true), ...);
            return (m_signature & required) == required;
        }

        // Efficient iteration over entities with specific components
        template<typename... Components>
        void forEach(std::function<void(EntityID, Components&...)> callback) {
            if (!hasComponents<Components...>()) return;

            auto componentArrays = std::make_tuple(getComponentArray<Components>()...);
            
            for (size_t i = 0; i < m_entities.size(); ++i) {
                EntityID entity = m_entities[i];
                callback(entity, std::get<ComponentArray<Components>*>(componentArrays)->at(i)...);
            }
        }

    private:
        ComponentSignature m_signature;
        ArchetypeID m_id;
        std::vector<EntityID> m_entities;
        std::unordered_map<ComponentTypeID, std::unique_ptr<IComponentArray>> m_componentArrays;

        static ArchetypeID generateID(const ComponentSignature& signature);
    };

    // View for efficient iteration over entities with specific components
    template<typename... Components>
    class ArchetypeView {
    public:
        ArchetypeView(ArchetypeRegistry& registry) : m_registry(registry) {
            collectMatchingArchetypes();
        }

        class Iterator {
        public:
            Iterator(ArchetypeView& view, size_t archetypeIndex, size_t entityIndex)
                : m_view(view), m_archetypeIndex(archetypeIndex), m_entityIndex(entityIndex) {
                skipToValid();
            }

            std::tuple<EntityID, Components&...> operator*() {
                auto& archetype = *m_view.m_matchingArchetypes[m_archetypeIndex];
                EntityID entity = archetype.getEntity(m_entityIndex);
                return std::forward_as_tuple(entity, archetype.template getComponent<Components>(m_entityIndex)...);
            }

            Iterator& operator++() {
                ++m_entityIndex;
                skipToValid();
                return *this;
            }

            bool operator!=(const Iterator& other) const {
                return m_archetypeIndex != other.m_archetypeIndex || m_entityIndex != other.m_entityIndex;
            }

        private:
            void skipToValid() {
                while (m_archetypeIndex < m_view.m_matchingArchetypes.size()) {
                    if (m_entityIndex < m_view.m_matchingArchetypes[m_archetypeIndex]->getEntityCount()) {
                        return; // Valid position found
                    }
                    ++m_archetypeIndex;
                    m_entityIndex = 0;
                }
            }

            ArchetypeView& m_view;
            size_t m_archetypeIndex;
            size_t m_entityIndex;
        };

        Iterator begin() { return Iterator(*this, 0, 0); }
        Iterator end() { return Iterator(*this, m_matchingArchetypes.size(), 0); }

        size_t size() const {
            size_t total = 0;
            for (const auto* archetype : m_matchingArchetypes) {
                total += archetype->getEntityCount();
            }
            return total;
        }

        bool empty() const { return m_matchingArchetypes.empty(); }

    private:
        void collectMatchingArchetypes() {
            m_matchingArchetypes.clear();
            
            ComponentSignature requiredSignature;
            ((requiredSignature[ComponentTypeRegistry::getTypeID<Components>() - 1] = true), ...);
            
            for (const auto& archetype : m_registry.getArchetypes()) {
                if ((archetype->getSignature() & requiredSignature) == requiredSignature) {
                    m_matchingArchetypes.push_back(archetype.get());
                }
            }
        }

        ArchetypeRegistry& m_registry;
        std::vector<Archetype*> m_matchingArchetypes;
    };

    // Main archetype registry for managing all archetypes
    class ArchetypeRegistry {
    public:
        ArchetypeRegistry() : m_nextEntityID(1) {}
        ~ArchetypeRegistry() = default;

        // Entity management
        EntityID createEntity();
        EntityID createEntity(const std::string& name);
        void destroyEntity(EntityID entity);
        bool isValid(EntityID entity) const;
        
        // Entity naming
        void setEntityName(EntityID entity, const std::string& name);
        const std::string& getEntityName(EntityID entity) const;

        // Component management
        template<typename T, typename... Args>
        T& addComponent(EntityID entity, Args&&... args) {
            auto* archetype = findOrCreateArchetype<T>(entity);
            size_t entityIndex = archetype->findEntity(entity);
            return archetype->template addComponent<T>(entityIndex, std::forward<Args>(args)...);
        }

        template<typename T>
        void removeComponent(EntityID entity) {
            // This requires moving entity to different archetype
            // Implementation would involve archetype transitions
            // For now, simplified version
        }

        template<typename T>
        bool hasComponent(EntityID entity) const {
            auto it = m_entityToArchetype.find(entity);
            if (it != m_entityToArchetype.end()) {
                ComponentTypeID typeID = ComponentTypeRegistry::getTypeID<T>();
                return it->second->getSignature()[typeID - 1];
            }
            return false;
        }

        template<typename T>
        T& getComponent(EntityID entity) {
            auto it = m_entityToArchetype.find(entity);
            auto* archetype = it->second;
            size_t entityIndex = archetype->findEntity(entity);
            return archetype->template getComponent<T>(entityIndex);
        }

        // View creation
        template<typename... Components>
        ArchetypeView<Components...> view() {
            return ArchetypeView<Components...>(*this);
        }
        
        template<typename... Components>
        ArchetypeView<Components...> view() const {
            return ArchetypeView<Components...>(const_cast<ArchetypeRegistry&>(*this));
        }

        // Get all archetypes (for view iteration)
        const std::vector<std::unique_ptr<Archetype>>& getArchetypes() const {
            return m_archetypes;
        }

        // Statistics
        size_t getEntityCount() const { return m_entityToArchetype.size(); }
        size_t getArchetypeCount() const { return m_archetypes.size(); }
        
        // Debug info
        void printDebugInfo() const;

    private:
        template<typename T>
        Archetype* findOrCreateArchetype(EntityID entity) {
            // Get current archetype of entity (if any)
            Archetype* currentArchetype = nullptr;
            auto it = m_entityToArchetype.find(entity);
            if (it != m_entityToArchetype.end()) {
                currentArchetype = it->second;
            }

            // Create new signature with added component
            ComponentSignature newSignature;
            if (currentArchetype) {
                newSignature = currentArchetype->getSignature();
            }
            ComponentTypeID typeID = ComponentTypeRegistry::getTypeID<T>();
            newSignature[typeID - 1] = true;

            // Find or create archetype with new signature
            auto* targetArchetype = getOrCreateArchetype(newSignature);

            // Move entity to new archetype if necessary
            if (currentArchetype != targetArchetype) {
                if (currentArchetype) {
                    // Remove from current archetype
                    size_t entityIndex = currentArchetype->findEntity(entity);
                    currentArchetype->removeEntity(entityIndex);
                }
                
                // Add to new archetype
                targetArchetype->addEntity(entity);
                m_entityToArchetype[entity] = targetArchetype;
            }

            return targetArchetype;
        }

        Archetype* getOrCreateArchetype(const ComponentSignature& signature);

        EntityID m_nextEntityID;
        std::vector<std::unique_ptr<Archetype>> m_archetypes;
        std::unordered_map<ArchetypeID, Archetype*> m_archetypeMap;
        std::unordered_map<EntityID, Archetype*> m_entityToArchetype;
        std::unordered_map<EntityID, std::string> m_entityNames;

        template<typename... Components>
        friend class ArchetypeView;
    };

} // namespace AstralEngine::ECS
