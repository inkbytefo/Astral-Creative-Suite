#pragma once
#include <glm/glm.hpp>

namespace AstralEngine {
    class Camera {
    public:
        Camera();

        // Transform setters
        void SetPosition(const glm::vec3& position) { 
            m_Position = position; 
            m_ViewDirty = true;
        }
        void SetTarget(const glm::vec3& target) { 
            m_Target = target; 
            m_ViewDirty = true;
        }
        
        // Perspective projection setup
        void SetPerspective(float fovRadians, float aspect, float nearPlane, float farPlane);
        
        // Transform getters
        const glm::vec3& GetPosition() const { return m_Position; }
        const glm::vec3& GetTarget() const { return m_Target; }
        const glm::vec3& GetUp() const { return m_Up; }
        
        // Camera basis vectors for frustum calculations
        glm::vec3 GetForward() const;
        glm::vec3 GetRight() const;
        glm::vec3 GetTrueUp() const;
        
        // Projection parameters
        float GetFOV() const { return m_FOVRadians; }
        float GetAspect() const { return m_Aspect; }
        float GetNearPlane() const { return m_NearPlane; }
        float GetFarPlane() const { return m_FarPlane; }
        
        // Matrix getters
        const glm::mat4& GetViewMatrix() const;
        const glm::mat4& GetProjectionMatrix() const;
        glm::mat4 GetViewProjectionMatrix() const;

    private:
        // Transform state
        glm::vec3 m_Position;
        glm::vec3 m_Target;
        glm::vec3 m_Up;
        
        // Projection state
        float m_FOVRadians = glm::radians(60.0f);  // Default 60° FOV
        float m_Aspect = 16.0f/9.0f;              // Default 16:9 aspect
        float m_NearPlane = 0.1f;                 // Default near plane
        float m_FarPlane = 100.0f;                // Default far plane

        // Cached matrices
        mutable glm::mat4 m_ViewMatrix;
        mutable glm::mat4 m_ProjectionMatrix;
        
        // Dirty flags for matrix recomputation
        mutable bool m_ViewDirty = true;
        mutable bool m_ProjectionDirty = true;
    };
}
