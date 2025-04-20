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

class Entity;
class ISystem;

class Engine {
public:
    bool Init();
    void Shutdown();
    bool IsRunning();
    float GetDeltaTime();

    void Run();
    void Update(float deltaTime);
    void Draw();

    // SystemManager
    bool AddSystem(ISystem* system);
    void RemoveSystem(ISystem* system); // Jury is still out on when I'd use this

    void AddEntity(Entity* entity);

private:
    void PollEvents();
    void ProcessEvent(const SDL_KeyboardEvent& event);
    void ProcessEvent(const SDL_MouseMotionEvent& event);
    void ProcessEvent(const SDL_MouseButtonEvent& event);
    void ProcessEvent(const SDL_WindowEvent& event);
    
private:
    Renderer mRenderer;
    UIManager mUIManager;
    std::vector<std::vector<ISystem*>> mSystems;
    std::vector<Entity*> mEntities;

    InputState mInputState;
};