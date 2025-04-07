#include "Game.h"
#include "Engine.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"

void Game::Run() {
    engine = std::make_unique<Engine>();
    if (engine->Init()) {
        // Main loop
        while (engine->IsRunning()) {
            engine->Run();

            float dt = engine->GetDeltaTime();
            Update(dt);
        }
    }
    engine->Shutdown();
}

void Game::Update(float dt) {
    // Game logic here
}