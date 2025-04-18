#pragma once

#include <glm/glm.hpp>
#include <glm/ext/quaternion_common.hpp>
#include <Renderer.h>

class Component {
public:
    virtual ~Component() = default;
};

class TransformComponent : public Component{
public:
    glm::vec3 mPosition = {0.0f, 0.0f, 0.0f};
    glm::vec3 mRotation = {0.0f, 0.0f, 0.0f}; // Euler angles in degrees
    glm::vec3 mScale = {1.0f, 1.0f, 1.0f};
};

class VelocityComponent : public Component{
public:
    glm::vec3 mVelocity = {0.0f, 0.0f, 0.0f};
    glm::vec3 mAngularVelocity = {0.0f, 0.0f, 0.0f};
};

class DisplayComponent : public Component{
public:
    Renderer::Mesh* mMesh;
};