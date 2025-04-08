#pragma once

class GameObject {
public:
    virtual ~GameObject() = default;
    virtual void Update() = 0;
    virtual void Render() = 0;
};