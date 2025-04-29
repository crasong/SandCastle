#pragma once

#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <glm/glm.hpp>
#include <Input.h>
#include <Render/RenderStructs.h>
#include <set>
#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <string>
#include <vector>
#include <unordered_map>

struct aiNode;
struct aiScene;
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

    struct TextureLoadingContext {
        aiTextureType          type = aiTextureType_NONE;
        std::string            filename;
        SDL_Surface*           imageData      = nullptr;
        SDL_GPUTransferBuffer* transferBuffer = nullptr;
    };

    struct MaterialLoadingContext {
        // this assumes PBR 
        std::string albedo;
        std::string normal;
        std::string emissive;
        std::string metallic;
        std::string roughness;
        std::string ao;
    };

    struct NodeLoadingContext {
        NodeLoadingContext() {}
        NodeLoadingContext(const aiNode* n) { pNode = n;}
        const aiNode* pNode = nullptr;
        bool isRequired = false;
    };

    struct MeshLoadingContext {
        // Scene info
        std::unordered_map<std::string, NodeLoadingContext> nodeInfoMap;
        std::unordered_map<std::string, TextureLoadingContext> textureInfoMap;
        std::vector<MaterialLoadingContext> materialInfos; // we use indices for identifying materials

        Assimp::Importer importer;
        const aiScene* scene;
    };

    struct ModelDescriptor {
        std::string foldername;
        std::string subFoldername;
        std::string fileExtension;
        std::string textureFilename;
        uint8_t samplerTypeIndex = 0;
        bool flipX = false;
        bool flipY = false;
        bool flipZ = false;
    };

    struct RenderPassContext {
        SDL_GPUCommandBuffer* commandBuffer = nullptr;
        SDL_GPUTexture* swapchainTexture = nullptr;
        //SDL_GPURenderPass* renderPass = nullptr;
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

    glm::vec2 GetWindowCenter() { return mCachedWindowCenter; }

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

    // returns nullptr if texture type not found
    static SDL_GPUTexture* GetTexture(const Mesh& mesh, const aiTextureType type) {
        for (auto& [filename, meshTexture] : mesh.textureIdMap) {
            if (meshTexture.type == type) return meshTexture.texture;
        }
        return nullptr;
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
        Mesh& mesh,
        SDL_GPUBufferCreateInfo& vertexBufferCreateInfo,
        SDL_GPUTransferBuffer*& vertexTransferBuffer,
        SDL_GPUBufferCreateInfo& indexBufferCreateInfo,
        SDL_GPUTransferBuffer*& indexTransferBuffer
    );
    bool CreateTextureGPUResources(
        const SDL_Surface* imageData,
        const std::string textureName,
        SDL_GPUTexture*& outTexture,
        SDL_GPUTransferBuffer*& outTransferBuffer
    );

    SDL_GPUShader* LoadShader(
        SDL_GPUDevice* device,
        const std::string& shaderFilename,
        const Uint32 samplerCount,
        const Uint32 uniformBufferCount,
        const Uint32 storageBufferCount,
        const Uint32 storageTextureCount);
    SDL_Surface* LoadImage(const ModelDescriptor& modelDescriptor, int desiredChannels = 0);
    SDL_Surface* LoadImage(const std::string& foldername, const std::string& subfoldername, const std::string& texturename, int desiredChannels = 0);
    SDL_Surface* LoadImageShared(SDL_Surface* image, int desiredChannels = 0);
    bool LoadModel(const ModelDescriptor& modelDescriptor, Mesh& outMesh, MeshLoadingContext& outContext);
    void ParseNodes(Mesh& outMesh, MeshLoadingContext& outContext);
    void ParseVertices(const aiScene* scene, const bool flipX, const bool flipY, const bool flipZ, Mesh& outMesh, MeshLoadingContext& outContext);
    void ParseMaterials(const aiScene* scene, Mesh& outMesh, MeshLoadingContext& outContext);
    void ParseTextures(const aiScene* scene, Mesh& outMesh, MeshLoadingContext& outContext);
    
private:
    SDL_Window* mWindow = nullptr;
    SDL_GPUDevice* mSDLDevice = nullptr;
    SDL_GPUTexture* mDepthTexture = nullptr;
    
    std::vector<SDL_GPUSampler*> mSamplers;
    std::unordered_map<RenderMode, SDL_GPUGraphicsPipeline*> mPipelines;
    std::unordered_map<std::string, Mesh> mMeshes;
    SDL_GPUGraphicsPipeline* mGridPipeline = nullptr;
    Mesh mGridMesh;

    std::vector<CameraNode*> mCameraNodes;
    std::vector<RenderNode*> mNodesThisFrame;

    Uint8 mCurrentSamplerIndex = 0;
    RenderMode mRenderMode = RenderMode::Fill;
    float mScale = 1.0f;
    glm::vec2 mCachedWindowCenter;
    const float mScaleStep = 10.0f;
    const float mCameraSpeed = 5.0f;
    const float mCameraRotationSpeed = 0.5f;

    friend class Engine;
    friend class RenderSystem;
};
