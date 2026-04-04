#include "camera.hpp"
#include <GLFW/glfw3.h>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

void Camera::updateVectors() {
    float radPitch = glm::radians(pitch); 
    float radYaw = glm::radians(yaw);
    
    front.x = cos(radPitch) * cos(radYaw);
    front.y = sin(radPitch);
    front.z = cos(radPitch) * sin(radYaw);
    front = glm::normalize(front);

    right = glm::cross(front, worldUp);
    right = glm::normalize(right);

    up = glm::cross(right, front);
    up = glm::normalize(up);
}

glm::mat4 Camera::getViewMatrix() {
    return glm::lookAt(pos, pos + front, up);
}

void Camera::rotate(glm::vec2 mouseDelta) {
    pitch += mouseDelta.y * sensitivity;
    yaw += mouseDelta.x * sensitivity;
    if (pitchLimit > 0) {
        if (pitch > pitchLimit) pitch = pitchLimit;
        else if (pitch < -pitchLimit) pitch = -pitchLimit;
    }
    updateVectors();
}

void Camera::move(CameraMovement direction, float timeDelta) {
    float velocity = timeDelta * speed;
    switch (direction) {
        case CameraMovement::FORWARD:
            pos += velocity * front;
            break;
        case CameraMovement::BACKWARD:
            pos -= velocity * front;
            break;
        case CameraMovement::LEFT:
            pos -= velocity * right;
            break;
        case CameraMovement::RIGHT:
            pos += velocity * right;
            break;
        case CameraMovement::UP:
            pos += velocity * up;
            break;
        case CameraMovement::DOWN:
            pos -= velocity * up;
            break;
    }
}

void Camera::zoom(float wheelDelta) {
    fov -= wheelDelta;
    if (fov < minFov) fov = minFov;
    else if (fov > maxFov) fov = maxFov;
}

Camera::Camera(
    glm::vec3 pos, 
    glm::vec3 worldUp, 
    float pitch,
    float pitchLimit,
    float yaw,
    float speed,
    float sensitivity,
    float fov,
    float minFov,
    float maxFov
)
    :pos(pos),
    worldUp(worldUp),
    pitch(pitch),
    pitchLimit(pitchLimit),
    yaw(yaw),
    speed(speed),
    sensitivity(sensitivity),
    fov(fov),
    minFov(minFov),
    maxFov(maxFov),
    mousePos(0.0f, 0.0f)
{   
    updateVectors();
}