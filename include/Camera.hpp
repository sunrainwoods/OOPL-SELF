#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <glm/vec2.hpp>

class Camera {
public:
    Camera();

    void SetPosition(const glm::vec2& position);
    const glm::vec2& GetPosition() const;

    glm::vec2 WorldToScreen(const glm::vec2& worldPos) const;
    bool IsInView(const glm::vec2& worldPos, float bufferX = 100.0f, float bufferY = 100.0f) const;
    void GetViewportBounds(float& left, float& right, float& bottom, float& top) const;

private:
    glm::vec2 m_Position = {0.0f, 0.0f};
};

#endif // CAMERA_HPP
