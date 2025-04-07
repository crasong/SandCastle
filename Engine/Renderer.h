#pragma once

#include <SDL3/SDL.h>

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
    SDL_Window* mWindow = nullptr;
    SDL_Renderer* mRenderer = nullptr;
};
