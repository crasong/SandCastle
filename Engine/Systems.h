#pragma once

#include <Nodes.h>
#include <vector>

class Entity;
class Renderer;
class UIManager;

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
protected:
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

class UISystem : public ISystem {
public:
    UISystem() = default;
    UISystem(UIManager* uiManager) : mUIManager(uiManager) {}
    ~UISystem() override = default;

    bool Init() override { return true; }
    void AddNodeForEntity(const Entity& entity) override;
    void Update(float deltaTime) override;
    void Shutdown() override {}
private:
    UIManager* mUIManager = nullptr;
    std::vector<UINode> mUINodes;
};

class CameraSystem : public ISystem {
public:
    CameraSystem() = default;
    CameraSystem(Renderer* renderer) : mRenderer(renderer) {}
    ~CameraSystem() override = default;

    bool Init() override { return true; }
    void AddNodeForEntity(const Entity& entity) override;
    void Update(float deltaTime) override;
    void Shutdown() override {}
private:
    static void SetOrthographicProjection(CameraComponent& outCamera,
        const float left, const float right, const float bottom, const float top, const float nearPlane, const float farPlane);
    static void SetPerspectiveProjection(CameraComponent& outCamera,
        const float fovY, const float aspectRatio, const float nearPlane, const float farPlane);
    static void SetViewDirection(CameraComponent& outCamera,
        const glm::vec3 position, const glm::vec3 direction, const glm::vec3 up = {0.0f, -1.0f, 0.0f});
    static void SetViewTarget(CameraComponent& outCamera,
        const glm::vec3 position, const glm::vec3 target, const glm::vec3 up = {0.0f, -1.0f, 0.0f});
    static void SetViewYXZ(CameraComponent& outCamera,
        const glm::vec3 position, const glm::vec3 rotation);

    Renderer* mRenderer = nullptr;
    std::vector<CameraNode> mCameraNodes;
};