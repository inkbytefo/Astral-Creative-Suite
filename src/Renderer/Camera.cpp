#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Astral {
    Camera::Camera() 
        : m_Position(2.0f, 2.0f, 2.0f), 
          m_Target(0.0f, 0.0f, 0.0f), 
          m_Up(0.0f, 0.0f, 1.0f) // Z-up koordinat sistemi varsayımı
    {
    }

    const glm::mat4& Camera::GetViewMatrix() const {
        if (m_IsDirty) {
            m_ViewMatrix = glm::lookAt(m_Position, m_Target, m_Up);
            m_IsDirty = false;
        }
        return m_ViewMatrix;
    }
}
