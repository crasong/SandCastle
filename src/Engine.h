#pragma once
#include <SDL3/SDL.h>

namespace Engine {
    bool Init(const char* title, int width, int height);
    void Shutdown();
    bool IsRunning();
    void PollEvents();
    void Present();
    float GetDeltaTime();
    SDL_Renderer* GetRenderer();
}