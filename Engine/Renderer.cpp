#include "Renderer.h"
#include <iostream>

Renderer::Renderer() {}

Renderer::~Renderer() {
    Shutdown();
}

bool Renderer::Init(const char* title, int width, int height) {
    mWindow = SDL_CreateWindow(title, width, height, SDL_WINDOW_RESIZABLE);
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