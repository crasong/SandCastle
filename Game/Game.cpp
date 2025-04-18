#include "Game.h"

void Game::Run() {
    mEngine = std::make_unique<Engine>();
    if (mEngine->Init()) {
        // Main loop
        while (mEngine->IsRunning()) {
            mEngine->Run();

            float dt = mEngine->GetDeltaTime();
            Update(dt);
        }
    }
    mEngine->Shutdown();
}

void Game::Update(float dt) {
    mEngine->Update(dt);
}