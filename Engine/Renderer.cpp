#include "Renderer.h"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;
#include <SDL3/SDL_vulkan.h>
#include <Volk/volk.h>

Renderer::Renderer() {}

Renderer::~Renderer() {
    Shutdown();
}

bool Renderer::Init(const char* title, int width, int height) {
    mWindow = SDL_CreateWindow(title, width, height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);
    if (!mWindow) {
        SDL_LogError(SDL_LOG_PRIORITY_ERROR, "SDL_CreateWindow failed: %s", SDL_GetError());
        return false;
    }

    mRenderer = SDL_CreateRenderer(mWindow, nullptr);
    if (!mRenderer) {
        SDL_LogError(SDL_LOG_PRIORITY_ERROR, "SDL_CreateRenderer failed: %s", SDL_GetError());
        return false;
    }

    return true;
}

/* To Init Vulkan, the following are needed:
1. Instance: the Vulkan API Context. This is where we enable extensions for things
    like validation layers, logging, and more. It is the root/global context of the app.
2. PhysicalDevice: enumerate the hardware for all 'GPUs', and query their features.
3. Device: the driver on the GPU. Extensions can be enabled for more functionality at
    the cost of speed. We can create multiple devices for 'multi-GPU' support
4. Swapchain: this is a list of 'images' that are needed to send 'images' to the screen.
    The number of 'images' in the Swapchain is what allows double or triple buffering.
    The Swapchain must be rebuilt if the image changes size or format.
*/
void Renderer::InitVulkan(SDL_Window* window) {
    SDL_Vulkan_LoadLibrary(nullptr);
    volkInitializeCustom((PFN_vkGetInstanceProcAddr)SDL_Vulkan_GetVkGetInstanceProcAddr());

    // Get required extensions from SDL
    uint32_t extCount = 0;
    auto requiredExtensions = SDL_Vulkan_GetInstanceExtensions(&extCount);

    vk::ApplicationInfo appInfo;
    appInfo.pApplicationName = "SandCastleGame";
    appInfo.pEngineName = "SandCastleEngine";
    appInfo.apiVersion = VK_API_VERSION_1_2;

    vk::InstanceCreateInfo createInfo;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = extCount;
    createInfo.ppEnabledExtensionNames = requiredExtensions;
    createInfo.flags = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;

    mInstance = vk::createInstance(createInfo);
    mDispatchLoader.init(mInstance);
    volkLoadInstanceOnly(mInstance);

    // We're assuming the first Physical Device is the one we want
    // But we probably want to make sure we're using the dGPU and not the iGPU
    mPhysicalDevice = mInstance.enumeratePhysicalDevices().front();


}

void Renderer::Clear() {
    SDL_SetRenderDrawColor(mRenderer, 0, 0, 0, 255);
    SDL_RenderClear(mRenderer);
}

void Renderer::Present() {
    SDL_RenderPresent(mRenderer);
    SDL_SetRenderDrawColor(mRenderer, 20, 20, 20, 255);
    SDL_RenderClear(mRenderer);
}

void Renderer::Shutdown() {
    if (mRenderer) SDL_DestroyRenderer(mRenderer);
    if (mWindow) SDL_DestroyWindow(mWindow);
}

SDL_Renderer* Renderer::GetRenderer() {
    return mRenderer;
}