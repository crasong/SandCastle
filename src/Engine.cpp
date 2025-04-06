#include "Engine.h"

static SDL_Window* window = nullptr;
static SDL_Renderer* renderer = nullptr;
static bool running = true;
static float deltaTime = 0.0f;
static uint64_t lastTicks = 0;

bool Engine::Init(const char* title, int width, int height) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_CAMERA | SDL_INIT_AUDIO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return false;
    }

    window = SDL_CreateWindow(title, width, height, SDL_WINDOW_RESIZABLE);
    renderer = SDL_CreateRenderer(window, nullptr);

    if (!window || !renderer) {
        SDL_Log("Window/Renderer creation failed: %s", SDL_GetError());
        return false;
    }

    lastTicks = SDL_GetTicks();
    return true;
}

void Engine::Shutdown() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

bool Engine::IsRunning() {
    return running;
}

void Engine::PollEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            running = false;
        }
    }

    // Delta time
    uint64_t currentTicks = SDL_GetTicks();
    deltaTime = (currentTicks - lastTicks) / 1000.0f;
    lastTicks = currentTicks;
}

float Engine::GetDeltaTime() {
    return deltaTime;
}

void Engine::Present() {
    SDL_RenderPresent(renderer);
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
    SDL_RenderClear(renderer);
}

SDL_Renderer* Engine::GetRenderer() {
    return renderer;
}
