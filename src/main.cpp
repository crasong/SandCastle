#include "Engine.h"
#include "Game.h"

int main(int argc, char* argv[]) {
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
