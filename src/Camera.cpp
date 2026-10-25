#include "Camera.hpp"
#include "config.hpp"
#include <cmath>

Camera::Camera()
    : m_Position(0.0f, 0.0f) {}

void Camera::SetPosition(const glm::vec2& position) {
    m_Position = position;
}

const glm::vec2& Camera::GetPosition() const {
    return m_Position;
}

glm::vec2 Camera::WorldToScreen(const glm::vec2& worldPos) const {
    return worldPos - m_Position;
}

bool Camera::IsInView(const glm::vec2& worldPos, float bufferX, float bufferY) const {
    const float cullDistX = static_cast<float>(WINDOW_WIDTH) * 0.5f + bufferX;
    const float cullDistY = static_cast<float>(WINDOW_HEIGHT) * 0.5f + bufferY;
    return (std::abs(worldPos.x - m_Position.x) < cullDistX &&
            std::abs(worldPos.y - m_Position.y) < cullDistY);
}

void Camera::GetViewportBounds(float& left, float& right, float& bottom, float& top) const {
    const float halfW = static_cast<float>(WINDOW_WIDTH) * 0.5f;
    const float halfH = static_cast<float>(WINDOW_HEIGHT) * 0.5f;
    left = m_Position.x - halfW;
    right = m_Position.x + halfW;
    bottom = m_Position.y - halfH;
    top = m_Position.y + halfH;
}
