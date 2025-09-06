#pragma once
#include <glm/glm.hpp>

namespace Astral {
    class Camera {
    public:
        Camera();

        void SetPosition(const glm::vec3& position) { m_Position = position; }
        void SetTarget(const glm::vec3& target) { m_Target = target; }

        const glm::mat4& GetViewMatrix() const;

    private:
        glm::vec3 m_Position;
        glm::vec3 m_Target;
        glm::vec3 m_Up;

        mutable glm::mat4 m_ViewMatrix;
        mutable bool m_IsDirty = true; // View matrisinin yeniden hesaplanması gerekip gerekmediğini izler
    };
}
