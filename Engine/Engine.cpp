#include "Engine.h"

static bool s_Running = true;
static float deltaTime = 0.0f;
static uint64_t lastTicks = 0;

bool Engine::Init() {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_CAMERA | SDL_INIT_AUDIO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return false;
    }

    if (!mRenderer.Init("SandCastle", 1980, 1080)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Engine: Renderer Init failed!");
        return false;
    }

    lastTicks = SDL_GetTicks();
    return true;
}

void Engine::Shutdown() {
    SDL_Quit();
}

bool Engine::IsRunning() {
    return s_Running;
}

float Engine::GetDeltaTime() {
    return deltaTime;
}

void Engine::Run() {
    PollEvents();

    //mRenderer.Clear();
    Draw();
}

void Engine::PollEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            s_Running = false;
        }
        if (event.type == SDL_EVENT_KEY_DOWN) {
            SDL_Keycode key = event.key.key;
            if (key == SDLK_ESCAPE) {
                s_Running = false;
            }
            if (key == SDLK_LEFT) {
                mRenderer.CycleRenderMode();
            }
            if (key == SDLK_RIGHT) {
                mRenderer.CycleSampler();
            }
            if (key == SDLK_UP) {
                mRenderer.IncreaseScale();
            }
            if (key == SDLK_DOWN) {
                mRenderer.DecreaseScale();
            }
        }
    }

    // Delta time
    uint64_t currentTicks = SDL_GetTicks();
    deltaTime = (currentTicks - lastTicks) / 1000.0f;
    lastTicks = currentTicks;
}

void Engine::Draw() {
    mRenderer.Present();
}

