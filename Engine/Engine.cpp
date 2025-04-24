#include "Engine.h"

#include <Entity.h>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui_impl_sdl3.h>
#include <Nodes.h>
#include <Systems.h>

static bool s_Running = true;
static float deltaTime = 0.0f;
static uint64_t lastTicks = 0;

bool Engine::Init() {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_CAMERA | SDL_INIT_AUDIO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return false;
    }

    if (!mRenderer.Init("SandCastle", 1980, 1080)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Engine: Renderer Init failed!");
        return false;
    }

    mUIManager.Init(mRenderer.mWindow, mRenderer.mSDLDevice);
    
    mSystems.resize(ISystem::SystemPriority::count);
    AddSystem(new MoveSystem());
    AddSystem(new CameraSystem(&mRenderer));
    AddSystem(new RenderSystem(&mRenderer));
    AddSystem(new UISystem(&mUIManager));

    // Make sample entity
    {
        Entity entity("Camera");
        entity.AddComponent<CameraComponent>();
        entity.AddComponent<TransformComponent>(
            glm::vec3(0.0f, 0.0f, -10.0f), 
            glm::vec3(0.0f, 0.0f, 0.0f), 
            glm::vec3(1.0f)
        );
        entity.AddComponent<VelocityComponent>(glm::vec3(), glm::vec3());
        entity.AddComponent<UIComponent>();
        AddEntity(new Entity(std::move(entity)));
    }
    {
        Entity entity("Space Helmet");
        entity.AddComponent<DisplayComponent>(&mRenderer.mMeshes["DamagedHelmet"]);
        entity.AddComponent<TransformComponent>(
            glm::vec3(3.0f, 0.0f, 0.0f), 
            glm::vec3(0.0f, 225.0f, 0.0f), 
            glm::vec3(1.0f));
        entity.AddComponent<VelocityComponent>(glm::vec3(), glm::vec3());
        entity.AddComponent<UIComponent>();
        AddEntity(new Entity(std::move(entity)));
    }
    {
        Entity entity("Sci Fi Helmet");
        entity.AddComponent<DisplayComponent>(&mRenderer.mMeshes["SciFiHelmet"]);
        entity.AddComponent<TransformComponent>(
            glm::vec3(-3.0f, 0.0f, 0.0f), 
            glm::vec3(0.0f, 135.0f, 0.0f), 
            glm::vec3(1.0f));
        entity.AddComponent<VelocityComponent>(glm::vec3(), glm::vec3());
        entity.AddComponent<UIComponent>();
        AddEntity(new Entity(std::move(entity)));
    }
    {
        Entity entity("Viking Room");
        entity.AddComponent<DisplayComponent>(&mRenderer.mMeshes["viking_room"]);
        entity.AddComponent<TransformComponent>(
            glm::vec3(0.0f, -7.0f, 2.0f), 
            glm::vec3(-90.0f, 90.0f, 0.0f), 
            glm::vec3(15.0f));
        entity.AddComponent<VelocityComponent>(glm::vec3(), glm::vec3());
        entity.AddComponent<UIComponent>();
        AddEntity(new Entity(std::move(entity)));
    }

    lastTicks = SDL_GetTicks();
    return true;
}

void Engine::Shutdown() {
    for (auto& systemList : mSystems) {
        for (auto& system : systemList) {
            if (system) {
                system->Shutdown();
                delete system;
            }
        }
    }
    for (auto& entity : mEntities) {
        delete entity;
    }
    mEntities.clear();
    SDL_Quit();
}

bool Engine::IsRunning() {
    return s_Running;
}

float Engine::GetDeltaTime() {
    return deltaTime;
}

void Engine::Run() {
    PollEvents();

    //Draw();
}

void Engine::PollEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);
        switch (event.type) {
            case SDL_EVENT_QUIT:
                s_Running = false;
                break;
            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP:
                ProcessEvent(event.key);
                break;
            case SDL_EVENT_MOUSE_MOTION:
                ProcessEvent(event.motion);
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                ProcessEvent(event.button);
                break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
                mInputState.mouseButtonDown[event.button.button] = false;
                mInputState.mouseDragging = false;
                break;
            case SDL_EVENT_MOUSE_WHEEL:
                mInputState.mouseScroll.x = event.wheel.x;
                mInputState.mouseScroll.y = event.wheel.y;
                mInputState.mousePosition.x = event.wheel.mouse_x;
                mInputState.mousePosition.y = event.wheel.mouse_y;
                break;
            case SDL_EVENT_WINDOW_RESIZED:
            case SDL_EVENT_WINDOW_EXPOSED:
            case SDL_EVENT_WINDOW_MINIMIZED:
            case SDL_EVENT_WINDOW_MAXIMIZED:
                mRenderer.ResizeWindow();
                break;
        }
    }

    // Delta time
    uint64_t currentTicks = SDL_GetTicks();
    deltaTime = (currentTicks - lastTicks) / 1000.0f;
    lastTicks = currentTicks;
}

void Engine::Draw() {
    // Render Passes handled by manager classes
    mRenderer.Render(&mUIManager);
}


bool Engine::AddSystem(ISystem* system) {
    if (system && system->Init()) {
        mSystems[system->mPriority].push_back(std::move(system));
        return true;
    }
    return false;
}

void Engine::Update(float deltaTime) {
    ProcessCameraInput(deltaTime, mRenderer.GetCameraEntity());
    mInputState.mouseScroll = { 0.0f, 0.0f };
    mInputState.mouseDelta = { 0.0f, 0.0f };

    for (uint8_t priority = 0; priority < ISystem::SystemPriority::count; ++priority) {
        for (auto& system : mSystems[priority]) {
            if (system) {
                system->Update(deltaTime);
            }
        }
    }
}

void Engine::RemoveSystem(ISystem* system) {
    for (auto& systemList : mSystems) {
        auto it = std::remove(systemList.begin(), systemList.end(), system);
        if (it != systemList.end()) {
            (*it)->Shutdown();
            systemList.erase(it, systemList.end());
            delete system;
            break;
        }
    }
}

void Engine::AddEntity(Entity* entity) {
    for (auto& systemList : mSystems) {
        for (auto& system : systemList) {
            if (system) {
                system->AddNodeForEntity(*entity);
            }
        }
    }
    mEntities.push_back(std::move(entity));
    mEntities.back()->PostRegistration();
}

void Engine::ProcessEvent(const SDL_KeyboardEvent& event) {
    // Handle keyboard events here
    SDL_Keycode key = event.key;
    mInputState.keyDown[key] = event.down;
    switch(key) {
        case SDLK_ESCAPE:
            s_Running = false;
            break;
        case SDLK_LALT:
            mInputState.altKeyDown = event.down;
            break;
        case SDLK_Z:
            if (event.down) {
                mRenderer.CycleRenderMode();
            }
            break;
        case SDLK_2:
            if (event.down) {
                mRenderer.CycleSampler();
            }
            break;
        case SDLK_UP:
            //mRenderer.IncreaseScale();
            break;
        case SDLK_DOWN:
            //mRenderer.DecreaseScale();
            break;
    }
}
void Engine::ProcessEvent(const SDL_MouseMotionEvent& event) {
    // Handle mouse motion events here
    mInputState.mouseDelta.x = event.x - mInputState.mousePosition.x;
    mInputState.mouseDelta.y = event.y - mInputState.mousePosition.y;
    mInputState.mousePosition.x = event.x;
    mInputState.mousePosition.y = event.y;
}
void Engine::ProcessEvent(const SDL_MouseButtonEvent& event) {
    // Handle mouse button events here
    mInputState.mouseButtonDown[event.button] = event.down;
    mInputState.mouseDragging = event.down && (event.button == SDL_BUTTON_LEFT);
}
void Engine::ProcessEvent(const SDL_WindowEvent& event) {
    // Handle window events here
}

void Engine::ProcessCameraInput(const float deltaTime, CameraNode* camera) {
    if (camera == nullptr) return;
    CameraComponent* cam = camera->mCamera;
    TransformComponent* transform = camera->mTransform;

    float camSpeed = 1.0f * deltaTime * cam->mDistance;
    float angleSpeed = 45.0f * deltaTime;
    float tiltSpeed = 30.0f * deltaTime;

    float yaw = transform->mRotation.y;
    glm::vec3 forwardDir{glm::sin(yaw), 0.0f, glm::cos(yaw)};
    glm::vec3 rightDir(forwardDir.z, 0.0f, -forwardDir.x);
    glm::vec3 upDir(cam->mUp);

    //glm::vec4 forward4 = glm::rotate(glm::mat4(1.0f), glm::radians(cam->mAngle), cam->mUp) * glm::vec4(0.0f, 0.0f, -1.0f, 1.0f);
    //glm::vec3 forward = glm::vec3(forward4.x, forward4.y, forward4.z);
    //glm::vec4 right4 = glm::rotate(glm::mat4(1.0f), glm::radians(cam->mAngle), cam->mUp) * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    //glm::vec3 right = glm::vec3(right4.x, right4.y, right4.z);

    glm::vec3 camVel(0.0f, 0.0f, 0.0f);
    if (mInputState.keyDown[SDLK_W]) {
        camVel += forwardDir;
    }
    if (mInputState.keyDown[SDLK_S]) {
        camVel -= forwardDir;
    }
    if (mInputState.keyDown[SDLK_A]) {
        camVel -= rightDir;
    }
    if (mInputState.keyDown[SDLK_D]) {
        camVel += rightDir;
    }
    if (mInputState.keyDown[SDLK_SPACE]) {
        camVel += upDir;
    }
    if (mInputState.keyDown[SDLK_LCTRL]) {
        camVel -= upDir;
    }

    if (glm::dot(camVel, camVel) > std::numeric_limits<float>::epsilon()) {
        transform->mPosition += glm::normalize(camVel) * camSpeed;
    }

    // cam->mTargetCenter += glm::normalize(camVel) * camSpeed;
    // if (glm::length(cam->mTargetCenter) > CAMERA_MAX_POSITION) {
    //     cam->mTargetCenter = glm::normalize(cam->mTargetCenter) * CAMERA_MAX_POSITION;
    // }

    // DecayTo(cam->mCenter  , cam->mTargetCenter  , 0.98f, deltaTime);
    // DecayTo(cam->mDistance, cam->mTargetDistance, 0.99f, deltaTime);
    // DecayTo(cam->mAngle   , cam->mTargetAngle   , 0.99f, deltaTime);
    // DecayTo(cam->mTilt    , cam->mTargetTilt    , 0.99f, deltaTime);

    //glm::vec4 toPos = glm::rotate(glm::mat4(1.0f), -cam->mTilt, right) * forward4;
    //transform->mPosition = cam->mCenter - cam->mDistance * glm::normalize(glm::vec3(toPos.x, toPos.y, toPos.z));
    
    if (mInputState.mouseButtonDown[SDL_BUTTON_LEFT]) {
        if (mInputState.mouseDragging) {
            if (mInputState.altKeyDown) {
                glm::vec2 mouseDelta = mInputState.mouseDelta * deltaTime * 5.0f;
                glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(mouseDelta.x), upDir);
                rotationMatrix = glm::rotate(rotationMatrix, glm::radians(mouseDelta.y), rightDir);
                // create direction vector from rotationMatrix
                glm::vec4 forward4 = rotationMatrix * glm::vec4(0.0f, 0.0f, -1.0f, 1.0f);
                glm::vec3 forward = glm::vec3(forward4.x, forward4.y, forward4.z);
    
                //float yaw = transform->mRotation.y;
                //glm::vec3 forwardDir{glm::sin(yaw), 0.0f, glm::cos(yaw)};
                //glm::vec3 rightDir(forwardDir.z, 0.0f, -forwardDir.x);
                //glm::vec3 upDir(cam->mUp);
    
                glm::vec3 orbitPos =  cam->mCenter - (forward * cam->mDistance);
                transform->mPosition = orbitPos;
            }
        }
    }
    else if (mInputState.mouseButtonDown[SDL_BUTTON_RIGHT]) {
        glm::vec2 mouseDelta = mInputState.mouseDelta * deltaTime * 5.0f;
        if (mInputState.altKeyDown) {
            transform->mPosition.x -= mouseDelta.x;
            transform->mPosition.y += mouseDelta.y;
        }
        else {
            transform->mRotation.x += mouseDelta.y;
            transform->mRotation.y += mouseDelta.x;
        }
    }
}

template<typename T>
void Engine::DecayTo(T& value, T target, float rate, float deltaTime) {
    // Decay to target value using exponential decay

    value = value + (target - value) * (1.0f - glm::pow(rate, 1000.0f * deltaTime));
}