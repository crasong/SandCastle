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
        glm::vec2 texCoord = {0, 0};
    };

    struct Mesh {
        std::vector<PositionTextureVertex> vertices;
        std::vector<Uint32> indices;
    };

public:
    Renderer();
    ~Renderer();

    bool Init(const char* title, int width, int height);
    void Clear();
    void Present();
    void Shutdown();


    void CycleRenderMode();
    void CycleSampler();

private:
    void InitAssetLoader();
    bool InitPipelines();

    SDL_GPUShader* LoadShader(
        SDL_GPUDevice* device,
        const std::string& shaderFilename,
        const Uint32 samplerCount,
        const Uint32 uniformBufferCount,
        const Uint32 storageBufferCount,
        const Uint32 storageTextureCount);
    SDL_Surface* LoadImage(const std::string& filename, int desiredChannels = 0);

    bool GPURenderPass(SDL_Window* window);
    
    #ifdef VULKAN_IMPLEMENTATION
    void InitVulkan(SDL_Window* window);
    #endif

private:
    SDL_Window* mWindow = nullptr;
    SDL_GPUDevice* mSDLDevice = nullptr;
    SDL_GPUBuffer* mVertexBuffer = nullptr;
    SDL_GPUBuffer* mIndexBuffer = nullptr;
    SDL_GPUTexture* mTexture = nullptr;
    std::vector<SDL_GPUSampler*> mSamplers;

    std::unordered_map<RenderMode, SDL_GPUGraphicsPipeline*> mPipelines;

    Uint8 mCurrentSamplerIndex = 0;
    RenderMode mRenderMode = RenderMode::Fill;
    Mesh mMesh;

    #ifdef VULKAN_IMPLEMENTATION
    vk::Instance mInstance;
    vk::PhysicalDevice mPhysicalDevice;
    vk::Device mDevice;
    vk::Queue mQueue;
    vk::SurfaceKHR mSurface;
    vk::detail::DispatchLoaderDynamic mDispatchLoader; // Not sure if I want to enforce passing this every time
    #endif
};
