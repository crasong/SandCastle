#pragma once

#include <SDL3/SDL.h>
#include <vulkan/vulkan.hpp>

class Renderer {
public:
    Renderer();
    ~Renderer();

    bool Init(const char* title, int width, int height);
    void Clear();
    void Present();
    void Shutdown();

    SDL_Renderer* GetRenderer();

private:
    void InitVulkan(SDL_Window* window);

    SDL_Window* mWindow = nullptr;
    SDL_Renderer* mRenderer = nullptr;

    vk::Instance mInstance;
    vk::PhysicalDevice mPhysicalDevice;
    vk::Device mDevice;
    vk::Queue mQueue;
    vk::SurfaceKHR mSurface;
    vk::detail::DispatchLoaderDynamic mDispatchLoader; // Not sure if I want to enforce passing this every time
};
