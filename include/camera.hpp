#pragma once
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

enum class CameraMovement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN,
};

class Camera {
public:
    glm::vec3       pos;
    glm::vec3       front;
    glm::vec3       up;
    glm::vec3       right;
    const glm::vec3 worldUp;
    float           speed;
    float           sensitivity;
    float           fov;
    float           minFov;
    float           maxFov;
    glm::vec2       mousePos;
    glm::mat4 getViewMatrix();
    void rotate(glm::vec2 mouseDelta);
    void move(CameraMovement direction, float timeDelta);
    void zoom(float wheelDelta);
    Camera(
        glm::vec3 pos, 
        glm::vec3 worldUp, 
        float pitch = 0.0f, 
        float pitchLimit = 89.0f, 
        float yaw = -90.0f, 
        float speed = 5.0f, 
        float sens = 0.025f, 
        float fov = 45.0f, 
        float minFov = 1.0f, 
        float maxFov = 45.0f
    );
private:    
    float           pitch;
    float           pitchLimit;
    float           yaw;    
    void updateVectors();
};