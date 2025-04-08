#include "Game.h"

#include "Engine.h"
#include "Player.h"

void Game::Run() {
    mEngine = std::make_unique<Engine>();
    if (mEngine->Init()) {
        // Init game components here
        mScene = std::make_unique<Scene>();
        mScene->AddObject(std::make_unique<Player>());

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
    // Game logic here
    mScene->Update();
    mScene->Render();
}