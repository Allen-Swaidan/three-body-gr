#include "camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

Camera::Camera(glm::vec3 pos, float yaw, float pitch)
    : position(pos), yaw(yaw), pitch(pitch),
      worldUp(0.0f, 1.0f, 0.0f),
      speed(30.0f), sensitivity(0.1f), fov(45.0f)
{
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(front);
    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}

glm::mat4 Camera::viewMatrix() const {
    return glm::lookAt(position, position + front, up);
}

void Camera::processKeyboard(int direction, float dt) {
    float v = speed * dt;
    switch (direction) {
        case FORWARD:  position += front * v; break;
        case BACKWARD: position -= front * v; break;
        case LEFT:     position -= right * v; break;
        case RIGHT:    position += right * v; break;
        case UP:       position += worldUp * v; break;
        case DOWN:     position -= worldUp * v; break;
    }
}

void Camera::processMouse(float xoffset, float yoffset) {
    xoffset *= sensitivity;
    yoffset *= sensitivity;
    yaw += xoffset;
    pitch += yoffset;
    pitch = glm::clamp(pitch, -89.0f, 89.0f);

    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(front);
    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}

void Camera::processScroll(float yoffset) {
    fov -= yoffset;
    fov = glm::clamp(fov, 1.0f, 90.0f);
}
