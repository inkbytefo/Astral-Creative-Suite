#include "ArchetypeECS.h"
#include "Core/Logger.h"
#include <cassert>
#include <functional> // For std::hash

namespace AstralEngine::ECS {

// Static member definition
ComponentTypeID ComponentTypeRegistry::s_componentCount = 0;

// Archetype implementation
Archetype::Archetype(const ComponentSignature& signature) 
    : m_signature(signature), m_id(generateID(signature)) {
}

size_t Archetype::addEntity(EntityID entity) {
    m_entities.push_back(entity);
    size_t entityIndex = m_entities.size() - 1;
    
    // Ensure all component arrays have space for this entity
    for (auto& [typeID, array] : m_componentArrays) {
        while (array->getSize() <= entityIndex) {
            // This will need proper component construction
            // For now, we'll handle this in the template methods
        }
    }
    
    return entityIndex;
}

void Archetype::removeEntity(size_t index) {
    assert(index < m_entities.size());
    
    // Remove from all component arrays (swap with last)
    for (auto& [typeID, array] : m_componentArrays) {
        array->removeEntity(index);
    }
    
    // Remove entity (swap with last)
    if (index < m_entities.size() - 1) {
        m_entities[index] = m_entities.back();
    }
    m_entities.pop_back();
}

EntityID Archetype::getEntity(size_t index) const {
    assert(index < m_entities.size());
    return m_entities[index];
}

size_t Archetype::findEntity(EntityID entity) const {
    auto it = std::find(m_entities.begin(), m_entities.end(), entity);
    if (it != m_entities.end()) {
        return std::distance(m_entities.begin(), it);
    }
    return SIZE_MAX; // Entity not found
}

ArchetypeID Archetype::generateID(const ComponentSignature& signature) {
    // Use std::hash for signature hashing
    std::string sigStr = signature.to_string();
    return std::hash<std::string>{}(sigStr);
}

// ArchetypeRegistry implementation
EntityID ArchetypeRegistry::createEntity() {
    EntityID entityID = m_nextEntityID++;
    
    // For new entities, create empty archetype (no components)
    ComponentSignature emptySignature;
    auto* archetype = getOrCreateArchetype(emptySignature);
    archetype->addEntity(entityID);
    m_entityToArchetype[entityID] = archetype;
    m_entityNames[entityID] = "Entity_" + std::to_string(entityID);
    
    return entityID;
}

EntityID ArchetypeRegistry::createEntity(const std::string& name) {
    EntityID entityID = createEntity();
    m_entityNames[entityID] = name;
    return entityID;
}

void ArchetypeRegistry::destroyEntity(EntityID entity) {
    auto it = m_entityToArchetype.find(entity);
    if (it != m_entityToArchetype.end()) {
        auto* archetype = it->second;
        size_t entityIndex = archetype->findEntity(entity);
        if (entityIndex != SIZE_MAX) {
            archetype->removeEntity(entityIndex);
        }
        m_entityToArchetype.erase(it);
    }
    
    // Remove entity name
    m_entityNames.erase(entity);
}

bool ArchetypeRegistry::isValid(EntityID entity) const {
    return entity != NULL_ENTITY && m_entityToArchetype.find(entity) != m_entityToArchetype.end();
}

void ArchetypeRegistry::setEntityName(EntityID entity, const std::string& name) {
    if (isValid(entity)) {
        m_entityNames[entity] = name;
    }
}

const std::string& ArchetypeRegistry::getEntityName(EntityID entity) const {
    static const std::string invalidName = "INVALID_ENTITY";
    auto it = m_entityNames.find(entity);
    return it != m_entityNames.end() ? it->second : invalidName;
}

void ArchetypeRegistry::printDebugInfo() const {
    AE_INFO("=== ArchetypeRegistry Debug Info ===");
    AE_INFO("Entities: {}", getEntityCount());
    AE_INFO("Archetypes: {}", getArchetypeCount());
    AE_INFO("Component Types Registered: {}", ComponentTypeRegistry::getRegisteredComponentCount());
    
    AE_INFO("Entity List:");
    for (const auto& [entity, archetype] : m_entityToArchetype) {
        const std::string& name = getEntityName(entity);
        size_t componentCount = archetype->getSignature().count();
        AE_INFO("  Entity {} ('{}'): {} components", entity, name, componentCount);
    }
    
    AE_INFO("Archetype Details:");
    for (const auto& archetype : m_archetypes) {
        AE_INFO("  Archetype {}: {} entities, signature: {}", 
                archetype->getID(), archetype->getEntityCount(), archetype->getSignature().to_string());
    }
    AE_INFO("=== End Debug Info ===");
}

Archetype* ArchetypeRegistry::getOrCreateArchetype(const ComponentSignature& signature) {
    // Calculate ID here instead of using private method
    std::string sigStr = signature.to_string();
    ArchetypeID id = std::hash<std::string>{}(sigStr);
    
    auto it = m_archetypeMap.find(id);
    if (it != m_archetypeMap.end()) {
        return it->second;
    }
    
    // Create new archetype
    auto archetype = std::make_unique<Archetype>(signature);
    Archetype* archetypePtr = archetype.get();
    
    m_archetypes.push_back(std::move(archetype));
    m_archetypeMap[id] = archetypePtr;
    
    AE_DEBUG("Created new archetype with signature: {} (ID: {})", 
             signature.to_string(), id);
    
    return archetypePtr;
}

// Template implementations are now inline in header for better performance

} // namespace AstralEngine::ECS
