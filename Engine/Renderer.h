#pragma once

#ifdef VULKAN_IMPLEMENTATION
#include <vulkan/vulkan.hpp>
#endif

#include <glm/glm.hpp>
#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <string>
#include <vector>
#include <unordered_map>

class Renderer {
public:
    enum RenderMode : Uint8 {
        Fill = 0,
        Line,
        count
    };

    struct PositionTextureVertex {
        glm::vec3 position = {0, 0, 0};
        glm::vec2 uv = {0, 0};
    };

    struct Mesh {
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
    };

public:
    Renderer();
    ~Renderer();

    bool Init(const char* title, int width, int height);
    void Clear();
    void Present();
    void Shutdown();

    void Resize();
    void CycleRenderMode();
    void CycleSampler();
    void IncreaseScale();
    void DecreaseScale();

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

    bool GPURenderPass(SDL_Window* window);
    
    #ifdef VULKAN_IMPLEMENTATION
    void InitVulkan(SDL_Window* window);
    #endif

private:
    SDL_Window* mWindow = nullptr;
    SDL_GPUDevice* mSDLDevice = nullptr;
    SDL_GPUTexture* mDepthTexture = nullptr;
    std::vector<SDL_GPUSampler*> mSamplers;

    std::unordered_map<RenderMode, SDL_GPUGraphicsPipeline*> mPipelines;

    Uint8 mCurrentSamplerIndex = 0;
    RenderMode mRenderMode = RenderMode::Fill;

    std::unordered_map<std::string, Mesh> mMeshes;

    float mScale = 1.0f;
    const float mScaleStep = 0.1f;

    float mCachedScreenAspectRatio = 1.0f;

    #ifdef VULKAN_IMPLEMENTATION
    vk::Instance mInstance;
    vk::PhysicalDevice mPhysicalDevice;
    vk::Device mDevice;
    vk::Queue mQueue;
    vk::SurfaceKHR mSurface;
    vk::detail::DispatchLoaderDynamic mDispatchLoader; // Not sure if I want to enforce passing this every time
    #endif
};
