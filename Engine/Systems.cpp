#include "Systems.h"

#include <Entity.h>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlgpu3.h>
#include <Renderer.h>

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
    {
        RenderNode node{};
        if (mRenderer->mMeshes.find("DamagedHelmet") != mRenderer->mMeshes.end()) {
            node.mDisplay = new DisplayComponent();
            node.mDisplay->mMesh = &(mRenderer->mMeshes["DamagedHelmet"]);\
            node.mTransform = new TransformComponent();
            node.mTransform->mPosition = {0.0f, 0.0f, 0.0f};
            node.mTransform->mRotation = {0.0f, 180.0f, 0.0f};
            node.mTransform->mScale = {0.5f, 0.5f, 0.5f};
            mRenderNodes.push_back(node);
        }
    }
    {
        RenderNode node{};
        if (mRenderer->mMeshes.find("viking_room") != mRenderer->mMeshes.end()) {
            node.mDisplay = new DisplayComponent();
            node.mDisplay->mMesh = &(mRenderer->mMeshes["viking_room"]);\
            node.mTransform = new TransformComponent();
            node.mTransform->mPosition = {0.0f, 0.0f, 0.0f};
            node.mTransform->mRotation = {0.0f, 0.0f, 0.0f};
            node.mTransform->mScale = {1.0f, 1.0f, 1.0f};
            mRenderNodes.push_back(node);
        }
    }
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
    mRenderer->mUIManager.BeginFrame();
    ImGui::Render();
    ImDrawData* drawData = ImGui::GetDrawData();
    const bool bUIMinimized = (drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f);

    SDL_GPUCommandBuffer* commandBuffer(SDL_AcquireGPUCommandBuffer(mRenderer->mSDLDevice));
    if (!commandBuffer) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_AcquireGPUCommandBuffer failed: %s", SDL_GetError());
        return;
    }

    SDL_GPUTexture* swapchainTexture;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, mRenderer->mWindow, &swapchainTexture, nullptr, nullptr)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_WaitAndAcquireGPUSwapchainTexture failed: %s", SDL_GetError());
        return;
    }
    
    if (swapchainTexture) {
        ImGui_ImplSDLGPU3_PrepareDrawData(drawData, commandBuffer);

        SDL_GPUColorTargetInfo colorTarget{};
        colorTarget.texture = swapchainTexture;
        colorTarget.store_op = SDL_GPU_STOREOP_STORE;
        colorTarget.load_op = SDL_GPU_LOADOP_CLEAR;
        colorTarget.clear_color = SDL_FColor{0.3f,0.2f,0.2f,1.0f};
        std::vector<SDL_GPUColorTargetInfo> colorTargets {colorTarget};

        SDL_GPUDepthStencilTargetInfo depthStencilTarget{};
        depthStencilTarget.texture = mRenderer->mDepthTexture;
        depthStencilTarget.clear_depth = 1.0f;
        depthStencilTarget.load_op = SDL_GPU_LOADOP_CLEAR;
        depthStencilTarget.store_op = SDL_GPU_STOREOP_STORE;
        depthStencilTarget.clear_stencil = 0;
        
        SDL_GPURenderPass* renderPass{
            SDL_BeginGPURenderPass(commandBuffer, colorTargets.data(), static_cast<Uint32>(colorTargets.size()), &depthStencilTarget)
        };

        for (auto& node : mRenderNodes) {
            const Renderer::Mesh& mesh = *(node.mDisplay->mMesh);
            const TransformComponent& transform = *(node.mTransform);

            SDL_BindGPUGraphicsPipeline(renderPass, mRenderer->mPipelines[mRenderer->mRenderMode]);
            std::vector<SDL_GPUBufferBinding> bindings{{mesh.vertexBuffer, 0}};
            SDL_BindGPUVertexBuffers(renderPass, 0, bindings.data(), static_cast<Uint32>(bindings.size()));
            SDL_GPUBufferBinding indexBufferBinding{mesh.indexBuffer, 0};
            SDL_BindGPUIndexBuffer(renderPass, &indexBufferBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
            SDL_GPUTextureSamplerBinding textureSamplerBinding{mesh.colorTexture, mRenderer->mSamplers[mRenderer->mCurrentSamplerIndex]};
            SDL_BindGPUFragmentSamplers(renderPass, 0, &textureSamplerBinding, 1);
    
            // projection matrix
            const float fovY = glm::radians(90.0f/mRenderer->mCachedScreenAspectRatio);
            glm::mat4 projectionMatrix = glm::perspective(fovY, mRenderer->mCachedScreenAspectRatio, 0.1f, 100.0f);
            // view matrix
            glm::mat4 viewMatrix = glm::lookAt(glm::vec3(0, -4, 0), glm::vec3(0, 0, 0), glm::vec3(0, 0, 1));
            // model matrix
            glm::mat4 modelMatrix = glm::mat4(1.0f);
            modelMatrix = glm::scale(modelMatrix, transform.mScale * glm::vec3(mRenderer->mScale));
            modelMatrix = glm::rotate(modelMatrix, glm::radians(transform.mRotation.y), glm::vec3(0, 1, 0));
            //modelMatrix = glm::rotate(modelMatrix, SDL_GetTicks() / 1000.0f, glm::vec3(0, 0, 1));
            modelMatrix = glm::translate(modelMatrix, transform.mPosition);
            // modelviewprojection matrix
            glm::mat4 mvpMatrix = (projectionMatrix * viewMatrix) * modelMatrix;
            SDL_PushGPUVertexUniformData(commandBuffer, 0, &mvpMatrix, sizeof(glm::mat4));
    
            SDL_DrawGPUIndexedPrimitives(renderPass, static_cast<Uint32>(mesh.indices.size()), 1, 0, 0, 0);
        }

        // Draw UI
        if (!bUIMinimized) {
            ImGui_ImplSDLGPU3_RenderDrawData(drawData, commandBuffer, renderPass);
        }

        SDL_EndGPURenderPass(renderPass);
    }

    if (!SDL_SubmitGPUCommandBuffer(commandBuffer)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_SubmitGPUCommandBuffer failed: %s", SDL_GetError());
        return;
    }
}