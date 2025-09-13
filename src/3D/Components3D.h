#pragma once

#include "ECS/Components.h"
#include "Asset/MeshAsset.h"
#include "Asset/MaterialAsset.h"
#include <glm/glm.hpp>
#include <memory>

namespace AstralEngine::D3 {

    // 3D Transform component for position, rotation, scale in 3D space
    struct Transform3D : public ECS::IComponent {
        glm::vec3 position{0.0f};
        glm::vec3 rotation{0.0f}; // Euler angles in radians
        glm::vec3 scale{1.0f};
        
        // Hierarchy support
        EntityID parent = INVALID_ENTITY;
        std::vector<EntityID> children;
        
        // Cached world matrix
        mutable glm::mat4 worldMatrix{1.0f};
        mutable bool dirty = true;
        
        Transform3D() = default;
        Transform3D(const glm::vec3& pos) : position(pos) {}
        Transform3D(const glm::vec3& pos, const glm::vec3& rot, const glm::vec3& scl)
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
        glm::mat4 getWorldMatrix(ECS::ArchetypeRegistry& registry) const;
        
        void markDirty() { dirty = true; }
    };

    // 3D Camera component for view and projection in 3D space
    struct Camera3D : public ECS::IComponent {
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
        
        Camera3D() = default;
        Camera3D(float fovDegrees, float aspect, float near, float far)
            : fov(glm::radians(fovDegrees)), aspectRatio(aspect), nearPlane(near), farPlane(far) {}
        
        glm::mat4 getViewMatrix(const Transform3D& transform) const {
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

    // 3D Mesh component for rendering 3D geometry
    struct Mesh3D : public ECS::IComponent {
        // Mesh data would be stored in asset system
        std::shared_ptr<class MeshAsset> meshAsset;
        
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
        
        Mesh3D() = default;
        explicit Mesh3D(std::shared_ptr<class MeshAsset> m) : meshAsset(m) {}
        
        void setMesh(std::shared_ptr<class MeshAsset> m) { meshAsset = m; }
        std::shared_ptr<class MeshAsset> getMesh() const { return meshAsset; }
    };

    // 3D Material component for surface properties
    struct Material3D : public ECS::IComponent {
        // Material properties would be stored in asset system
        std::shared_ptr<class MaterialAsset> materialAsset;
        
        // Optional property overrides
        glm::vec3 albedo{1.0f};
        float metallic = 0.0f;
        float roughness = 0.5f;
        glm::vec3 emissive{0.0f};
        
        Material3D() = default;
        explicit Material3D(std::shared_ptr<class MaterialAsset> m) : materialAsset(m) {}
        
        void setMaterial(std::shared_ptr<class MaterialAsset> m) { materialAsset = m; }
        std::shared_ptr<class MaterialAsset> getMaterial() const { return materialAsset; }
    };

}