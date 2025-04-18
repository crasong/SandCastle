#pragma once

#include <glm/glm.hpp>
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
        SDL_GPUBuffer* vertexBuffer = nullptr;
        SDL_GPUBuffer* indexBuffer = nullptr;
        SDL_GPUTexture* colorTexture = nullptr;
    };

    struct ModelDescriptor {
        std::string foldername;
        std::string meshFilename;
        std::string textureFilename;
        bool flipX = false;
        bool flipY = false;
        bool flipZ = false;
    };

public:
    Renderer();
    ~Renderer();

    bool Init(const char* title, int width, int height);
    void Clear();
    void Render(UIManager* uiManager);
    void Shutdown();

    void Resize();
    void CycleRenderMode();
    void CycleSampler();
    void IncreaseScale();
    void DecreaseScale();

    void SetCameraEntity(CameraNode* cameraNode);
    void SubmitNode(RenderNode* node) {
        mNodesThisFrame.push_back(node);
    }

private:
    void InitAssetLoader();
    bool InitPipelines();
    void InitSamplers();
    void InitMeshes();
    bool InitMesh(const ModelDescriptor& modelDescriptor, Mesh& mesh);

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
    SDL_GPUTexture* mDepthTexture = nullptr;
    
    std::vector<SDL_GPUSampler*> mSamplers;
    std::unordered_map<RenderMode, SDL_GPUGraphicsPipeline*> mPipelines;
    std::unordered_map<std::string, Mesh> mMeshes;

    //CameraNode* mCameraNode = nullptr;
    std::vector<CameraNode*> mCameraNodes;
    std::vector<RenderNode*> mNodesThisFrame;
    SDL_GPUCommandBuffer* mCommandBuffer = nullptr;
    SDL_GPUTexture* mSwapchainTexture = nullptr;
    SDL_GPURenderPass* mRenderPass = nullptr;

    Uint8 mCurrentSamplerIndex = 0;
    RenderMode mRenderMode = RenderMode::Fill;
    float mScale = 1.0f;
    const float mScaleStep = 0.1f;

    friend class Engine;
    friend class RenderSystem;
};
