#define SDL_MAIN_HANDLED
#include "Engine.h"
#include "Game.h"
//#include <SDL3/SDL.h>

int main(int argc, char* argv[]) {
    // if (!SDL_Init(SDL_INIT_VIDEO)) {
    //     SDL_Log("SDL_Init failed: %s", SDL_GetError());
    //     return 1;
    // }

    // SDL_Window* win = SDL_CreateWindow("Hello", 800, 600, SDL_WINDOW_OPENGL);
    // if (!win) {
    //     SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
    //     SDL_Quit();
    //     return 1;
    // }

    // SDL_Delay(2000);  // Keep window open for 2s
    // SDL_DestroyWindow(win);
    // SDL_Quit();
    // return 0;
    if (!Engine::Init("SandCastle Engine", 1280, 720)) return -1;

    Game game;

    while (Engine::IsRunning()) {
        Engine::PollEvents();
        game.Update(Engine::GetDeltaTime());
        game.Render();
        Engine::Present();
    }

    Engine::Shutdown();
    return 0;
}
