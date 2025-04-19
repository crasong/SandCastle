#pragma once

#include <functional>
#include <glm/glm.hpp>
#include <glm/ext/quaternion_common.hpp>
#include <Interfaces.h>
#include <Renderer.h>
#include <vector>

class Entity;
using GetViewablesFunc = std::function<void(std::vector<IUIViewable*>&)>;

class Component : public IUIViewable{
public:
    virtual ~Component() = default;
    virtual void BeginFrame() override {
        // Default implementation does nothing
    }
protected:
    const Entity* mEntity = nullptr;
    friend class Entity;
};

class TransformComponent : public Component{
public:
    TransformComponent() = default;
    explicit TransformComponent(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale)
        : mPosition(position), mRotation(rotation), mScale(scale) {}

    void BeginFrame() override;

    glm::vec3 mPosition = {0.0f, 0.0f, 0.0f};
    glm::vec3 mRotation = {0.0f, 0.0f, 0.0f}; // Euler angles in degrees
    glm::vec3 mScale = {1.0f, 1.0f, 1.0f};
};

class VelocityComponent : public Component{
public:
    VelocityComponent() = default;
    explicit VelocityComponent(const glm::vec3& velocity, const glm::vec3& angularVelocity)
        : mVelocity(velocity), mAngularVelocity(angularVelocity) {}

    void BeginFrame() override;

    glm::vec3 mVelocity = {0.0f, 0.0f, 0.0f};
    glm::vec3 mAngularVelocity = {0.0f, 0.0f, 0.0f};
};

class DisplayComponent : public Component{
public:
    DisplayComponent() = default;
    explicit DisplayComponent(Renderer::Mesh* mesh) : mMesh(mesh) {}

    void BeginFrame() override;

    Renderer::Mesh* mMesh = nullptr;
};

class CameraComponent : public Component{
public:
    CameraComponent() = default;
    void BeginFrame() override;

    enum CameraMode : Uint8 {
        FirstPerson = 0,
        ThirdPerson,
        FreeCamera,
        count
    };

    float mFOV = 90.0f; // Field of view in degrees
    float mOrthoSize = 10.0f; // Orthographic size
    float mAspectRatio = 16.0f/9.0f; // Aspect ratio
    float mNearPlane = 0.1f; // Near clipping plane distance
    float mFarPlane = 100.0f; // Far clipping plane distance
    glm::vec3 mUp = {0.0f, 0.0f, 1.0f}; // Up vector
    glm::vec3 mLookAt = {0.0f, 0.0f, 0.0f}; // Look-at point
    CameraMode mCameraMode = ThirdPerson; // Camera mode
    Renderer::ProjectionMode mProjectionMode = Renderer::ProjectionMode::Perspective;
    Renderer::ViewPort mViewPort = {{0, 0}, {0, 0}}; // Viewport dimensions
};

class UIComponent : public Component{
    public:
    UIComponent() = default;

    void BeginFrame() override;
    void BeginFrameForViewables();
};