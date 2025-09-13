#pragma once

#include "ArchetypeECS.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <vector>
#include <limits>
#include <algorithm>

// Forward declarations
namespace AstralEngine {
    class UnifiedMaterialInstance;
    class ModelAsset;
}

namespace AstralEngine::ECS {

    // Forward declaration for ArchetypeRegistry access
    class ArchetypeRegistry;

    // Transform component for position, rotation, scale
    struct Transform : public IComponent {
        glm::vec3 position{0.0f};
        glm::vec3 rotation{0.0f}; // Euler angles in radians
        glm::vec3 scale{1.0f};
        
        // Hierarchy support
        EntityID parent = INVALID_ENTITY;
        std::vector<EntityID> children;
        
        // Cached world matrix (updated by transform system)
        mutable glm::mat4 worldMatrix{1.0f};
        mutable bool dirty = true;
        
        Transform() = default;
        Transform(const glm::vec3& pos) : position(pos) {}
        Transform(const glm::vec3& pos, const glm::vec3& rot, const glm::vec3& scl)
            : position(pos), rotation(rot), scale(scl) {}
        
        // Calculate local transformation matrix
        glm::mat4 getLocalMatrix() const {
            glm::mat4 t = glm::translate(glm::mat4(1.0f), position);
            glm::mat4 r = glm::rotate(glm::mat4(1.0f), rotation.x, glm::vec3(1, 0, 0));
            r = glm::rotate(r, rotation.y, glm::vec3(0, 1, 0));
            r = glm::rotate(r, rotation.z, glm::vec3(0, 0, 1));
            glm::mat4 s = glm::scale(glm::mat4(1.0f), scale);
            return t * r * s;
        }
        
        // Calculate world transformation matrix
        glm::mat4 getWorldMatrix(ArchetypeRegistry& registry) const;
        
        void markDirty() { dirty = true; }
    };

    // Render component for entities that should be drawn
    struct RenderComponent : public IComponent {
        std::shared_ptr<ModelAsset> modelAsset;
        
        // Optional material override (for special cases)
        std::shared_ptr<UnifiedMaterialInstance> materialOverride;
        
        // Rendering flags
        bool visible = true;
        bool castShadows = true;
        bool receiveShadows = true;
        
        // LOD (Level of Detail) settings
        float lodBias = 1.0f;
        float maxDistance = std::numeric_limits<float>::max();
        
        // Culling bounds (local space)
        glm::vec3 boundingBoxMin{-1.0f};
        glm::vec3 boundingBoxMax{1.0f};
        float boundingSphereRadius = 1.0f;
        
        RenderComponent() = default;
        explicit RenderComponent(std::shared_ptr<ModelAsset> m) : modelAsset(m) {}
        RenderComponent(std::shared_ptr<ModelAsset> m, std::shared_ptr<UnifiedMaterialInstance> override)
            : modelAsset(m), materialOverride(override) {}
        
        // Helper methods
        void setModel(std::shared_ptr<ModelAsset> m) { modelAsset = m; }
        std::shared_ptr<ModelAsset> getModel() const { return modelAsset; }
        
        // Get effective material for a submesh (override or from model)
        std::shared_ptr<UnifiedMaterialInstance> getEffectiveMaterial(uint32_t submeshIndex = 0) const;
    };

    // Camera component for view and projection
    struct Camera : public IComponent {
        // Projection parameters
        float fov = 45.0f;
        float aspectRatio = 16.0f / 9.0f;
        float nearPlane = 0.1f;
        float farPlane = 1000.0f;
        
        // Camera type
        enum class Type { Perspective, Orthographic };
        Type type = Type::Perspective;
        
        // Orthographic parameters
        float orthoSize = 10.0f;
        
        // Viewport
        glm::vec4 viewport{0.0f, 0.0f, 1.0f, 1.0f}; // x, y, width, height (normalized)
        
        // Cached matrices
        mutable glm::mat4 viewMatrix{1.0f};
        mutable glm::mat4 projectionMatrix{1.0f};
        mutable bool viewDirty = true;
        mutable bool projectionDirty = true;
        
        Camera() = default;
        Camera(float fovDegrees, float aspect, float near, float far)
            : fov(glm::radians(fovDegrees)), aspectRatio(aspect), nearPlane(near), farPlane(far) {}
        
        glm::mat4 getViewMatrix(const Transform& transform) const {
            if (viewDirty) {
                // Calculate view matrix from transform
                glm::vec3 position = transform.position;
                glm::vec3 forward = glm::normalize(glm::vec3(
                    cos(transform.rotation.y) * cos(transform.rotation.x),
                    sin(transform.rotation.x),
                    sin(transform.rotation.y) * cos(transform.rotation.x)
                ));
                glm::vec3 up = glm::vec3(0, 1, 0);
                viewMatrix = glm::lookAt(position, position + forward, up);
                viewDirty = false;
            }
            return viewMatrix;
        }
        
        glm::mat4 getProjectionMatrix() const {
            if (projectionDirty) {
                if (type == Type::Perspective) {
                    projectionMatrix = glm::perspective(fov, aspectRatio, nearPlane, farPlane);
                } else {
                    float halfSize = orthoSize * 0.5f;
                    projectionMatrix = glm::ortho(-halfSize * aspectRatio, halfSize * aspectRatio,
                                                -halfSize, halfSize, nearPlane, farPlane);
                }
                projectionDirty = false;
            }
            return projectionMatrix;
        }
        
        void markDirty() { viewDirty = projectionDirty = true; }
    };

    // Light component for illumination
    struct Light : public IComponent {
        enum class Type { Directional, Point, Spot };
        
        Type type = Type::Point;
        glm::vec3 color{1.0f};
        float intensity = 1.0f;
        glm::vec3 direction{0.0f, -1.0f, 0.0f}; // Default downward direction
        
        // Point/Spot light parameters
        float range = 10.0f;
        float attenuation = 1.0f;
        
        // Spot light parameters
        float innerConeAngle = 30.0f; // degrees
        float outerConeAngle = 45.0f; // degrees
        
        // Shadow casting
        bool castShadows = false;
        
        Light() = default;
        Light(Type t, const glm::vec3& col, float intens)
            : type(t), color(col), intensity(intens) {}
    };

    // Name component for debugging and identification
    struct Name : public IComponent {
        std::string name;
        
        Name() = default;
        Name(const std::string& n) : name(n) {}
    };

    // Tag component for grouping entities
    struct Tag : public IComponent {
        std::string tag;
        
        Tag() = default;
        Tag(const std::string& t) : tag(t) {}
    };

} // namespace AstralEngine::ECS
