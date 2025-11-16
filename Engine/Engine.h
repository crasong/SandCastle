#pragma once

#include <Input.h>
#include <Interfaces.h>
#include <memory>
#include <Renderer.h>
#include <typeindex>
#include <typeinfo>
#include <UIManager.h>
#include <unordered_map>
#include <vector>

class CameraNode;
class Entity;
class ISystem;

class Engine {
public:
    bool Init();
    void Shutdown();
    bool IsRunning() const { return mRunning; }
    float GetDeltaTime() const { return mDeltaTime; }

    void Run();
    void Update(float deltaTime);
    void Draw();

    // SystemManager
    template<typename T, typename... Args>
    bool AddSystem(Args&&... args) {
        static_assert(std::is_base_of<ISystem, T>::value, "T must derive from ISystem");
        auto system = std::make_unique<T>(std::forward<Args>(args)...);
        if (system && system->Init()) {
            mSystems[system->mPriority].push_back(std::move(system));
            return true;
        }
        return false;
    }

    std::unique_ptr<Entity> CreateEntity(const std::string& name = "");
    void DestroyEntity(uint32_t entityID);

    template<typename T>
    void DecayTo(T& value, T target, float rate, float deltaTime);

private:
    void PollEvents();
    void ProcessEvent(const SDL_KeyboardEvent& event);
    void ProcessEvent(const SDL_MouseMotionEvent& event);
    void ProcessEvent(const SDL_MouseButtonEvent& event);
    void ProcessEvent(const SDL_WindowEvent& event);

    void ProcessCameraInput(const float deltaTime, CameraNode* camera);
    void AddEntityInternal(std::unique_ptr<Entity> entity);

private:
    Renderer mRenderer;
    UIManager mUIManager;
    std::vector<std::vector<std::unique_ptr<ISystem>>> mSystems;
    std::vector<std::unique_ptr<Entity>> mEntities;

    InputState mInputState;
    bool mRunning = true;
    float mDeltaTime = 0.0f;
    uint64_t mLastTicks = 0;
};