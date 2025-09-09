#pragma once

#include <memory>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
// Legacy Entity system - kept for backward compatibility
// For new code, use ECS::Scene and ECS::Components

// Forward declarations to avoid circular includes
namespace AstralEngine {
    class Model;
    class UnifiedMaterialInstance;
}

namespace AstralEngine {

    class LegacyTransform {
    public:
        LegacyTransform() : m_position(0.0f), m_rotation(0.0f, 0.0f, 0.0f), m_scale(1.0f) {}

        // Position
        void setPosition(const glm::vec3& position) { m_position = position; m_dirty = true; }
        void setPosition(float x, float y, float z) { m_position = glm::vec3(x, y, z); m_dirty = true; }
        const glm::vec3& getPosition() const { return m_position; }

        // Rotation
        void setRotation(const glm::vec3& rotation) { m_rotation = rotation; m_dirty = true; }
        void setRotation(float x, float y, float z) { m_rotation = glm::vec3(x, y, z); m_dirty = true; }
        const glm::vec3& getRotation() const { return m_rotation; }

        // Scale
        void setScale(const glm::vec3& scale) { m_scale = scale; m_dirty = true; }
        void setScale(float x, float y, float z) { m_scale = glm::vec3(x, y, z); m_dirty = true; }
        void setScale(float scale) { m_scale = glm::vec3(scale); m_dirty = true; }
        const glm::vec3& getScale() const { return m_scale; }

        // Matrix operations
        const glm::mat4& getTransformMatrix() const {
            if (m_dirty) {
                recalculateMatrix();
            }
            return m_transformMatrix;
        }

        // Transform operations
        void translate(const glm::vec3& translation) { m_position += translation; m_dirty = true; }
        void rotate(const glm::vec3& rotation) { m_rotation += rotation; m_dirty = true; }
        void scale(const glm::vec3& scale) { m_scale *= scale; m_dirty = true; }

    private:
        void recalculateMatrix() const {
            glm::mat4 translation = glm::translate(glm::mat4(1.0f), m_position);
            glm::mat4 rotation = glm::toMat4(glm::quat(m_rotation));
            glm::mat4 scale = glm::scale(glm::mat4(1.0f), m_scale);
            m_transformMatrix = translation * rotation * scale;
            m_dirty = false;
        }

        mutable glm::mat4 m_transformMatrix = glm::mat4(1.0f);
        mutable bool m_dirty = true;

        glm::vec3 m_position;
        glm::vec3 m_rotation;
        glm::vec3 m_scale;
    };

    class Entity {
    public:
        Entity() = default;
        ~Entity() = default;

        // Unique ID for the entity
        uint32_t getId() const { return m_id; }

        // Transform component (always present)
        LegacyTransform& getTransform() { return m_transform; }
        const LegacyTransform& getTransform() const { return m_transform; }

        // Name for debugging
        void setName(const std::string& name) { m_name = name; }
        const std::string& getName() const { return m_name; }

        // Legacy render component methods - deprecated
        // Use ECS::RenderComponent with the new ECS system instead

    private:
        static uint32_t s_nextId;
        uint32_t m_id = s_nextId++;
        std::string m_name = "Entity";

        LegacyTransform m_transform;
    };
}