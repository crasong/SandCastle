#pragma once

#include <imgui.h>
#include <SDL3/SDL_gpu.h>

class UIManager {
public:
    UIManager();
    ~UIManager();

    // Initialize ImGui
    void Init(SDL_Window* window, SDL_GPUDevice* device);

    // Start a new ImGui frame
    void BeginFrame();

    // Render ImGui
    void Render(SDL_Window* window, SDL_GPUDevice* device);

    // Cleanup ImGui
    void Shutdown();
protected:
    
};