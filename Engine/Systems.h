#pragma once

#include <Components.h>
#include <Nodes.h>
#include <vector>

class Entity;
class Renderer;

class ISystem {
public:
    enum SystemPriority : uint8_t {
        High = 0,
        Medium,
        Low,
        count
    };
    virtual ~ISystem() = default;
    virtual bool Init() = 0;
    virtual void AddNodeForEntity(const Entity& entity) = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Shutdown() = 0;
private:
    friend class Engine;
    SystemPriority mPriority = SystemPriority::Medium;
};

class MoveSystem : public ISystem {
public:
    MoveSystem() = default;
    ~MoveSystem() override = default;

    bool Init() override { return true; }
    void AddNodeForEntity(const Entity& entity) override;
    void Update(float deltaTime) override;
    void Shutdown() override {}
private:
    std::vector<MoveNode> mMoveNodes;
};

class RenderSystem : public ISystem {
public:
    RenderSystem() = default;
    RenderSystem(Renderer* renderer) : mRenderer(renderer) {}
    ~RenderSystem() override = default;

    bool Init() override;
    void AddNodeForEntity(const Entity& entity) override;
    void Update(float deltaTime) override;
    void Shutdown() override {}
private:
    Renderer* mRenderer = nullptr;
    std::vector<RenderNode> mRenderNodes;
};