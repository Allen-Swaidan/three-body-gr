#pragma once
#include <glm/glm.hpp>

class Camera {
public:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;

    float yaw;
    float pitch;
    float speed;
    float sensitivity;
    float fov;

    Camera(glm::vec3 pos = glm::vec3(0.0f, 30.0f, 60.0f),
           float yaw = -90.0f, float pitch = -25.0f);

    glm::mat4 viewMatrix() const;
    void processKeyboard(int direction, float dt);
    void processMouse(float xoffset, float yoffset);
    void processScroll(float yoffset);

    enum Movement { FORWARD, BACKWARD, LEFT, RIGHT, UP, DOWN };
};
