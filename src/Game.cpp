#include "Game.h"
#include "Engine.h"

void Game::Update(float dt) {
    // Game logic here
}

void Game::Render() {
    SDL_Renderer* renderer = Engine::GetRenderer();
    SDL_SetRenderDrawColor(renderer, 100, 255, 100, 255);
    SDL_FRect rect = { 100, 100, 200, 150 };
    SDL_RenderFillRect(renderer, &rect);
}
