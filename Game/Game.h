#pragma once
#include <Engine.h>
#include <memory>

class Game {
public:
    ~Game();

    void Run();

    void Update(float dt);
private:
    std::unique_ptr<Engine> mEngine;
};
