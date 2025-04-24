#include "Systems.h"

#include <Entity.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlgpu3.h>
#include <Renderer.h>
#include <UIManager.h>

void MoveSystem::AddNodeForEntity(const Entity& entity) {
    TransformComponent* transform = entity.GetComponent<TransformComponent>();
    VelocityComponent* velocity = entity.GetComponent<VelocityComponent>();
    if (transform && velocity) {
        MoveNode node;
        node.mTransform = transform;
        node.mVelocity = velocity;
        mMoveNodes.push_back(node);
    }
}

void MoveSystem::Update(float deltaTime) {
    for (auto& node : mMoveNodes) {
        auto& transform = node.mTransform;
        auto& velocity = node.mVelocity;

        transform->mPosition += velocity->mVelocity * deltaTime;
        transform->mRotation += velocity->mAngularVelocity * deltaTime;
    }
}

bool RenderSystem::Init() {
    
    return true;
}

void RenderSystem::AddNodeForEntity(const Entity& entity) {
    DisplayComponent* display = entity.GetComponent<DisplayComponent>();
    TransformComponent* transform = entity.GetComponent<TransformComponent>();
    if (display && transform) {
        RenderNode node;
        node.mDisplay = display;
        node.mTransform = transform;
        mRenderNodes.push_back(node);
    }
}

void RenderSystem::Update(float deltaTime) {
    for (auto& node : mRenderNodes) {
        mRenderer->SubmitNode(&node);
    }
}

void UISystem::AddNodeForEntity(const Entity& entity) {
    UIComponent* ui = entity.GetComponent<UIComponent>();
    if (ui) {
        UINode node;
        node.mUI = ui;
        mUINodes.push_back(node);
    }
}
void UISystem::Update(float deltaTime) {
    for (auto& node : mUINodes) {
        mUIManager->SubmitNode(&node);
    }
}

void CameraSystem::AddNodeForEntity(const Entity& entity) {
    CameraComponent* camera = entity.GetComponent<CameraComponent>();
    TransformComponent* transform = entity.GetComponent<TransformComponent>();
    if (camera && transform) {
        CameraNode node;
        node.mCamera = camera;
        node.mTransform = transform;
        mCameraNodes.push_back(node);
        // if (mRenderer) {
        //     mRenderer->SetCameraEntity(&mCameraNodes.back());
        // }
    }
}

void CameraSystem::Update(float deltaTime) {
    if (mRenderer) {
        mRenderer->SetCameraEntity(&mCameraNodes[0]);
    }
    for (auto& node : mCameraNodes) {
        auto& camera = node.mCamera;
        auto& transform = node.mTransform;
        if (camera->mCameraMode == Renderer::ProjectionMode::Perspective) {
            SetPerspectiveProjection(*camera, camera->mFOV, camera->mAspectRatio, camera->mNearPlane, camera->mFarPlane);
        } else {
            float halfWidth = camera->mOrthoSize * camera->mAspectRatio;
            float halfHeight = camera->mOrthoSize;
            SetOrthographicProjection(*camera, -halfWidth, halfWidth, -halfHeight, halfHeight, camera->mNearPlane, camera->mFarPlane);
        }
        if (camera->mCameraMode == CameraComponent::CameraMode::FirstPerson) {
            SetViewYXZ(*camera, transform->mPosition, transform->mRotation);
        } 
        else if (camera->mCameraMode == CameraComponent::CameraMode::ThirdPerson) {
            SetViewTarget(*camera, transform->mPosition, camera->mCenter, camera->mUp);
        }
    }
}

void CameraSystem::SetOrthographicProjection(CameraComponent& outCamera,
                                            const float left, const float right, 
                                            const float bottom, const float top, 
                                            const float near, const float far) {
    outCamera.mProjectionMatrix = glm::mat4{1.f};
    outCamera.mProjectionMatrix[0][0] = 2.f / (right - left);
    outCamera.mProjectionMatrix[1][1] = 2.f / (bottom - top);
    outCamera.mProjectionMatrix[2][2] = 1.f / (far - near);
    outCamera.mProjectionMatrix[3][0] = -(right + left) / (right - left);
    outCamera.mProjectionMatrix[3][1] = -(bottom + top) / (bottom - top);
    outCamera.mProjectionMatrix[3][2] = -near / (far - near);
}
void CameraSystem::SetPerspectiveProjection(CameraComponent& outCamera,
                                            const float fovY, const float aspectRatio, 
                                            const float nearPlane, const float farPlane) {
    //outCamera.mProjectionMatrix = glm::perspective(fovY, aspectRatio, nearPlane, farPlane);
    SDL_assert(glm::abs(aspectRatio - std::numeric_limits<float>::epsilon()) > 0.0f);
    const float tanHalfFovy = glm::tan(fovY / 2.0f);
    outCamera.mProjectionMatrix = glm::mat4{0.f};
    outCamera.mProjectionMatrix[0][0] = 1.0f / (aspectRatio * tanHalfFovy);
    outCamera.mProjectionMatrix[1][1] = 1.0f / (tanHalfFovy);
    outCamera.mProjectionMatrix[2][2] = farPlane / (farPlane - nearPlane);
    outCamera.mProjectionMatrix[2][3] = 1.0f;
    outCamera.mProjectionMatrix[3][2] = -(farPlane * nearPlane) / (farPlane - nearPlane);
    //outCamera.mProjectionMatrix[1][1] *= -1; // flip Y axis for OpenGL
}

void CameraSystem::SetViewDirection(CameraComponent& outCamera,
                                    const glm::vec3 position, 
                                    const glm::vec3 direction, 
                                    const glm::vec3 up) {
        const glm::vec3 w{glm::normalize(direction)};
        const glm::vec3 u{glm::normalize(glm::cross(w, up))};
        const glm::vec3 v{glm::cross(w, u)};
      
        outCamera.mViewMatrix = glm::mat4{1.f};
        outCamera.mViewMatrix[0][0] = u.x;
        outCamera.mViewMatrix[1][0] = u.y;
        outCamera.mViewMatrix[2][0] = u.z;
        outCamera.mViewMatrix[0][1] = v.x;
        outCamera.mViewMatrix[1][1] = v.y;
        outCamera.mViewMatrix[2][1] = v.z;
        outCamera.mViewMatrix[0][2] = w.x;
        outCamera.mViewMatrix[1][2] = w.y;
        outCamera.mViewMatrix[2][2] = w.z;
        outCamera.mViewMatrix[3][0] = -glm::dot(u, position);
        outCamera.mViewMatrix[3][1] = -glm::dot(v, position);
        outCamera.mViewMatrix[3][2] = -glm::dot(w, position);
}
void CameraSystem::SetViewTarget(CameraComponent& outCamera,
                                 const glm::vec3 position, 
                                 const glm::vec3 target, 
                                 const glm::vec3 up) {
    SetViewDirection(outCamera, position, target - position, up);
}
void CameraSystem::SetViewYXZ(CameraComponent& outCamera,const glm::vec3 position, const glm::vec3 rotation) {
    const float c3 = glm::cos(rotation.z);
    const float s3 = glm::sin(rotation.z);
    const float c2 = glm::cos(rotation.x);
    const float s2 = glm::sin(rotation.x);
    const float c1 = glm::cos(rotation.y);
    const float s1 = glm::sin(rotation.y);
    const glm::vec3 u{(c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1)};
    const glm::vec3 v{(c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3)};
    const glm::vec3 w{(c2 * s1), (-s2), (c1 * c2)};
    outCamera.mViewMatrix = glm::mat4{1.f};
    outCamera.mViewMatrix[0][0] = u.x;
    outCamera.mViewMatrix[1][0] = u.y;
    outCamera.mViewMatrix[2][0] = u.z;
    outCamera.mViewMatrix[0][1] = v.x;
    outCamera.mViewMatrix[1][1] = v.y;
    outCamera.mViewMatrix[2][1] = v.z;
    outCamera.mViewMatrix[0][2] = w.x;
    outCamera.mViewMatrix[1][2] = w.y;
    outCamera.mViewMatrix[2][2] = w.z;
    outCamera.mViewMatrix[3][0] = -glm::dot(u, position);
    outCamera.mViewMatrix[3][1] = -glm::dot(v, position);
    outCamera.mViewMatrix[3][2] = -glm::dot(w, position);
}