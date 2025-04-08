#pragma once
#include "Engine.h"
#include "Scene.h"
#include <memory>

class Game {
public:
    void Run();

    void Update(float dt);
private:
    std::unique_ptr<Engine> mEngine;
    std::unique_ptr<Scene> mScene;
};
