#pragma once

#include <glm/glm.hpp>
#include <Input.h>
#include <Render/RenderStructs.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <string>
#include <vector>
#include <unordered_map>

class CameraNode;
class RenderNode;
class UIManager;

class Renderer {
public:
    enum RenderMode : Uint8 {
        Fill = 0,
        Line,
        count
    };

    enum ProjectionMode : Uint8 {
        Perspective = 0,
        Orthographic,
        numModes
    };

    struct ViewPort {
        glm::vec2 position = {0, 0};
        glm::vec2 size = {0, 0};
    };

    struct PositionTextureVertex {
        glm::vec3 position = {0, 0, 0};
        glm::vec2 uv = {0, 0};
    };

    struct Mesh {
        glm::vec3 mRootPosition = {0.0f, 0.0f, 0.0f}; // Base position in local space
        glm::vec3 mRootOrientation = {0.0f, 0.0f, 0.0f}; // Base orientation in degrees
        glm::vec3 mRootScale = {1.0f, 1.0f, 1.0f};
        std::vector<PositionTextureVertex> vertices;
        std::vector<Uint32> indices;
        uint8_t samplerTypeIndex = 0;
        SDL_GPUBuffer* vertexBuffer = nullptr;
        SDL_GPUBuffer* indexBuffer = nullptr;
        SDL_GPUTexture* colorTexture = nullptr;
    };

    struct ModelDescriptor {
        std::string foldername;
        std::string meshFilename;
        std::string textureFilename;
        uint8_t samplerTypeIndex = 0;
        bool flipX = false;
        bool flipY = false;
        bool flipZ = false;
    };

    struct RenderPassContext {
        SDL_GPUCommandBuffer* commandBuffer = nullptr;
        SDL_GPUTexture* swapchainTexture = nullptr;
        SDL_GPURenderPass* renderPass = nullptr;
        CameraGPU cameraData{};
    };

public:
    Renderer();
    ~Renderer();

    bool Init(const char* title, int width, int height);
    void Clear();
    void Render(UIManager* uiManager);
    void Shutdown();

    void ResizeWindow();
    void CycleRenderMode();
    void CycleSampler();
    void IncreaseScale();
    void DecreaseScale();

    void SetCameraEntity(CameraNode* cameraNode);
    CameraNode* GetCameraEntity() const {
        if (mCameraNodes.size() > 0) {
            return mCameraNodes[0];
        }
        return nullptr;
    }
    void SubmitNode(RenderNode* node) {
        mNodesThisFrame.push_back(node);
    }

private:
    void InitAssetLoader();
    bool InitPipelines();
    void InitSamplers();
    void InitGrid();
    void InitMeshes();
    bool InitMesh(const ModelDescriptor& modelDescriptor, Mesh& mesh);

    // Render pass functions
    bool BeginRenderPass(RenderPassContext& context);
    void InitCameraData(const CameraNode* cameraNode, CameraGPU& outCameraData) const;
    void RecordGridCommands(RenderPassContext& context);
    void RecordModelCommands(RenderPassContext& context);
    void RecordUICommands(RenderPassContext& context);
    void EndRenderPass(RenderPassContext& context);

    bool CreateModelGPUResources(
        const ModelDescriptor& modelDescriptor,
        Mesh& mesh,
        SDL_GPUBufferCreateInfo& vertexBufferCreateInfo,
        SDL_GPUTransferBuffer*& vertexTransferBuffer,
        SDL_GPUBufferCreateInfo& indexBufferCreateInfo,
        SDL_GPUTransferBuffer*& indexTransferBuffer
    );
    bool CreateTextureGPUResources(
        const ModelDescriptor& modelDescriptor,
        Mesh& mesh,
        SDL_Surface*& imageData,
        SDL_GPUTextureCreateInfo& textureCreateInfo,
        SDL_GPUTransferBuffer*& textureTransferBuffer
    );

    std::string GetFolderName(const std::string& filename) const;

    SDL_GPUShader* LoadShader(
        SDL_GPUDevice* device,
        const std::string& shaderFilename,
        const Uint32 samplerCount,
        const Uint32 uniformBufferCount,
        const Uint32 storageBufferCount,
        const Uint32 storageTextureCount);
    SDL_Surface* LoadImage(const ModelDescriptor& modelDescriptor, int desiredChannels = 0);
    bool LoadModel(const ModelDescriptor& modelDescriptor, Renderer::Mesh& outMesh);
    
private:
    SDL_Window* mWindow = nullptr;
    SDL_GPUDevice* mSDLDevice = nullptr;
    SDL_GPUTexture* mColorTexture = nullptr;
    SDL_GPUTexture* mDepthTexture = nullptr;
    
    std::vector<SDL_GPUSampler*> mSamplers;
    std::unordered_map<RenderMode, SDL_GPUGraphicsPipeline*> mPipelines;
    std::unordered_map<std::string, Mesh> mMeshes;
    SDL_GPUGraphicsPipeline* mGridPipeline = nullptr;
    Mesh mGridMesh;

    std::vector<CameraNode*> mCameraNodes;
    std::vector<RenderNode*> mNodesThisFrame;

    //SDL_GPUCommandBuffer* mCommandBuffer = nullptr;
    //SDL_GPUTexture* mSwapchainTexture = nullptr;
    //SDL_GPURenderPass* mRenderPass = nullptr;

    Uint8 mCurrentSamplerIndex = 0;
    RenderMode mRenderMode = RenderMode::Fill;
    float mScale = 1.0f;
    const float mScaleStep = 10.0f;
    const float mCameraSpeed = 5.0f;
    const float mCameraRotationSpeed = 0.5f;

    friend class Engine;
    friend class RenderSystem;
};
