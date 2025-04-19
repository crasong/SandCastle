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
    // mUIManager->BeginFrame();
    // ImGui::Render();
    // ImDrawData* drawData = ImGui::GetDrawData();
    // const bool bUIMinimized = (drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f);

    // for (auto& node : mUINodes) {
    //     node.mUI->BeginFrame();
    // }
    for (auto& node : mUINodes) {
        mUIManager->SubmitNode(&node);
    }
}