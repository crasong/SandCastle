#include "Game.h"
#include "Engine.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"

void Game::Update(float dt) {
    // Game logic here
}

void Game::Render() {
    SDL_Renderer* renderer = Engine::GetRenderer();
    SDL_SetRenderDrawColor(renderer, 100, 255, 100, 255);
    SDL_FRect rect = { 100, 100, 200, 150 };
    SDL_RenderFillRect(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, 63, 127, 255, 255);
    SDL_FRect rect2 = { 880, 540, 300, 300 };
    SDL_RenderFillRect(renderer, &rect2);
}
