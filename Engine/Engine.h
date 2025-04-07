#pragma once
#include "Renderer.h"

class Engine {
public:
    bool Init();
    void Shutdown();
    bool IsRunning();
    float GetDeltaTime();

    void Run();
private:
    void PollEvents();
    void Draw();

private:
    Renderer mRenderer;
};