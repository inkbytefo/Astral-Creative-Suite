#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <cassert>

namespace AstralEngine {
    Camera::Camera() 
        : m_Position(2.0f, 2.0f, 2.0f), 
          m_Target(0.0f, 0.0f, 0.0f), 
          m_Up(0.0f, 0.0f, 1.0f) // Z-up koordinat sistemi varsayımı
    {
    }

    void Camera::SetPerspective(float fovRadians, float aspect, float nearPlane, float farPlane) {
        m_FOVRadians = fovRadians;
        m_Aspect = aspect;
        m_NearPlane = nearPlane;
        m_FarPlane = farPlane;
        m_ProjectionDirty = true;
    }

    glm::vec3 Camera::GetForward() const {
        glm::vec3 forward = m_Target - m_Position;
        float length = glm::length(forward);
        
        // Guard against degenerate cases
        if (length < 1e-6f) {
            // If position == target, return default forward vector
            return glm::vec3(1.0f, 0.0f, 0.0f);
        }
        
        return forward / length;
    }

    glm::vec3 Camera::GetRight() const {
        glm::vec3 forward = GetForward();
        glm::vec3 right = glm::cross(forward, m_Up);
        float length = glm::length(right);
        
        // Guard against degenerate up/forward (parallel vectors)
        if (length < 1e-6f) {
            // Fallback: use world Y if up is parallel to forward
            right = glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f));
            length = glm::length(right);
            if (length < 1e-6f) {
                // If still degenerate, use world X
                return glm::vec3(1.0f, 0.0f, 0.0f);
            }
        }
        
        return right / length;
    }

    glm::vec3 Camera::GetTrueUp() const {
        glm::vec3 forward = GetForward();
        glm::vec3 right = GetRight();
        return glm::normalize(glm::cross(right, forward));
    }

    const glm::mat4& Camera::GetViewMatrix() const {
        if (m_ViewDirty) {
            m_ViewMatrix = glm::lookAt(m_Position, m_Target, m_Up);
            m_ViewDirty = false;
        }
        return m_ViewMatrix;
    }

    const glm::mat4& Camera::GetProjectionMatrix() const {
        if (m_ProjectionDirty) {
            m_ProjectionMatrix = glm::perspective(m_FOVRadians, m_Aspect, m_NearPlane, m_FarPlane);
            m_ProjectionDirty = false;
        }
        return m_ProjectionMatrix;
    }

    glm::mat4 Camera::GetViewProjectionMatrix() const {
        return GetProjectionMatrix() * GetViewMatrix();
    }
}
