#pragma once

#include <functional>
#include <glm/ext/quaternion_common.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <Interfaces.h>
#include <Renderer.h>
#include <vector>

#define CAMERA_MAX_DIST 8000.0f
#define CAMERA_MIN_TILT 15.0f
#define CAMERA_MAX_TILT 89.0f
#define CAMERA_MAX_POSITION 7000.0f

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

    bool mShow = true;
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

    float mDistance = 5.0f;
    float mTargetDistance = 5.0f;
    float mTilt = 45.0f; // Tilt angle in degrees
    float mTargetTilt = 45.0f; // Target tilt angle in degrees
    float mAngle = 45.0f; // Angle in degrees
    float mTargetAngle = 45.0f; // Target angle in degrees

    float mFOV = 45.0f; // Field of view in degrees
    float mOrthoSize = 10.0f; // Orthographic size
    float mAspectRatio = 16.0f/9.0f; // Aspect ratio
    float mNearPlane = 0.1f; // Near clipping plane distance
    float mFarPlane = 1000.0f; // Far clipping plane distance
    glm::vec3 mUp = {0.0f, 1.0f, 0.0f}; // Up vector
    glm::vec3 mCenter = {0.0f, 0.0f, 0.0f}; // Center point for third-person camera
    glm::vec3 mTargetCenter = {0.0f, 0.0f, 0.0f}; // Target center point for third-person camera
    CameraMode mCameraMode = FirstPerson; // Camera mode
    Renderer::ProjectionMode mProjectionMode = Renderer::ProjectionMode::Perspective;
    Renderer::ViewPort mViewPort = {{0, 0}, {0, 0}}; // Viewport dimensions

    glm::mat4 mViewMatrix = glm::mat4(1.0f); // View matrix
    glm::mat4 mProjectionMatrix = glm::mat4(1.0f); // Projection matrix
};

class UIComponent : public Component{
    public:
    UIComponent() = default;

    void BeginFrame() override;
    void BeginFrameForViewables();
};