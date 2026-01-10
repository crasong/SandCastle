#include "Game.h"
#include <Entity.h>
#include <Systems.h>

Game::~Game() = default;

void Game::Run() {
    mEngine = std::make_unique<Engine>();
    if (mEngine->Init()) {
        // Main loop
        while (mEngine->IsRunning()) {
            mEngine->Run();

            float dt = mEngine->GetDeltaTime();
            Update(dt);

            mEngine->Draw();
        }
    }
    mEngine->Shutdown();
}

void Game::Update(float dt) {
    mEngine->Update(dt);
}