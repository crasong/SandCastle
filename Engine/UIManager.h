#pragma once

#include <imgui.h>
#include <SDL3/SDL_gpu.h>
#include <vector>

class UINode;

class UIManager {
public:
    UIManager();
    ~UIManager();

    // Initialize ImGui
    void Init(SDL_Window* window, SDL_GPUDevice* device);

    // Start a new ImGui frame
    void BeginFrame();

    // Render ImGui
    void Render(SDL_GPUCommandBuffer* command_buffer, SDL_GPUTexture* swapchain_texture);
    // Cleanup ImGui
    void Shutdown();

    // Submit a node for rendering
    // This is used to submit UI nodes that need to be rendered in the current frame
    void SubmitNode(UINode* node) {
        mNodesThisFrame.push_back(node);
    }
protected:
    void DockSpaceUI();
    void ToolbarUI();
    const float toolbarSize = 50;
    float mMenuBarHeight = 10.0f;

    std::vector<UINode*> mNodesThisFrame;
};