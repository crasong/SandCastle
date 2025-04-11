#pragma once

#ifdef VULKAN_IMPLEMENTATION
#include <vulkan/vulkan.hpp>
#endif

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <string>
#include <vector>
#include <unordered_map>

class Renderer {
public:
    Renderer();
    ~Renderer();

    bool Init(const char* title, int width, int height);
    void Clear();
    void Present();
    void Shutdown();

    enum RenderMode : Uint8 {
        Fill = 0,
        Line,
        count
    };

    void CycleRenderMode();

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

    bool GPURenderPass(SDL_Window* window);
    
    #ifdef VULKAN_IMPLEMENTATION
    void InitVulkan(SDL_Window* window);
    #endif

private:
    SDL_Window* mWindow = nullptr;
    SDL_GPUDevice* mSDLDevice;

    std::unordered_map<RenderMode, SDL_GPUGraphicsPipeline*> mPipelines;

    RenderMode mRenderMode = RenderMode::Fill;

    #ifdef VULKAN_IMPLEMENTATION
    vk::Instance mInstance;
    vk::PhysicalDevice mPhysicalDevice;
    vk::Device mDevice;
    vk::Queue mQueue;
    vk::SurfaceKHR mSurface;
    vk::detail::DispatchLoaderDynamic mDispatchLoader; // Not sure if I want to enforce passing this every time
    #endif
};
