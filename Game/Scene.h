#pragma once

#include <vector>
#include <memory>
#include "GameObject.h"

class Scene {
public:
    void AddObject(std::unique_ptr<GameObject> obj);
    void Update();
    void Render();

private:
    std::vector<std::unique_ptr<GameObject>> objects;
};