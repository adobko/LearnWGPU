#pragma once
#include <glm/glm.hpp>
#include <array>

// Each vec3 in a WGSL uniform struct occupies 16 bytes (vec3 + 4B pad).

struct CameraUniforms {
    glm::vec3 viewPos;
    float _pad0; // brings total to 16 bytes
};

// ---- Lights ----

struct DirLightUBO {
    glm::vec3 dir;      float _pad0;
    glm::vec3 ambient;  float _pad1;
    glm::vec3 diffuse;  float _pad2;
    glm::vec3 specular; float _pad3;
};

struct PointLightUBO {
    glm::vec3 pos;      float _pad0;
    glm::vec3 ambient;  float _pad1;
    glm::vec3 diffuse;  float _pad2;
    glm::vec3 specular; float _pad3;
    float constant;
    float linear;
    float quadratic;
    float _pad4; // pad to 16-byte multiple (struct size = 80B)
};

struct PointLightsArrayUBO {
    std::array<PointLightUBO, 4> lights;
};

struct SpotLightUBO {
    glm::vec3 pos;      float _pad0;
    glm::vec3 front;    float _pad1;
    glm::vec3 ambient;  float _pad2;
    glm::vec3 diffuse;  float _pad3;
    glm::vec3 specular; float _pad4;
    float constant;
    float linear;
    float quadratic;
    float cutOff;
    float outerCutOff;
    float _pad5[3]; // pad struct to 16-byte multiple
};

struct MaterialUniforms {
    float shininess;
    float _pad[3]; // pad to 16 bytes minimum
};