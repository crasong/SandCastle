#pragma once
#include <stdint.h>

class IRenderable {
public:
    virtual ~IRenderable() = default;
    virtual void Render() = 0;
};

class IMoveable {
public:
    virtual ~IMoveable() = default;
    virtual void Move(float deltaTime) = 0;
};