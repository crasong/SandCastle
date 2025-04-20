#pragma once

#include <glm/glm.hpp>
#include <SDL3/SDL.h>
#include <unordered_map>

struct InputState {
    bool mouseButtonDown[SDL_BUTTON_X2] = { false };
    //bool mMouseButtonUp[SDL_BUTTON_X2] = { false };
    //bool mMouseButtonPressed[SDL_BUTTON_X2] = { false };
    bool mouseDragging = false;
    bool altKeyDown = false;
    glm::vec2 mousePosition = { 0.0f, 0.0f };
    glm::vec2 mouseDelta = { 0.0f, 0.0f };
    glm::vec2 mouseScroll = { 0.0f, 0.0f };
    std::unordered_map<SDL_Keycode, bool> keyDown{
        {SDLK_W, false},
        {SDLK_A, false},
        {SDLK_S, false},
        {SDLK_D, false},
    };
};
