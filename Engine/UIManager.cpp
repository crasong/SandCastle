#include "UIManager.h"

#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlgpu3.h>
#include <Nodes.h>
#include <SDL3/SDL.h>

// statics
static bool show_demo_window = false;
static bool show_another_window = false;
static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

UIManager::UIManager() {
    // Constructor implementation (if needed)
}

UIManager::~UIManager() {}

void UIManager::Init(SDL_Window* window, SDL_GPUDevice* device) {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer bindings
    ImGui_ImplSDL3_InitForSDLGPU(window);
    ImGui_ImplSDLGPU3_InitInfo init_info{};
    init_info.Device = device;
    init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(device, window);
    init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
    ImGui_ImplSDLGPU3_Init(&init_info);
}

void UIManager::BeginFrame() {
    ImGui_ImplSDLGPU3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    ImGuiIO& io = ImGui::GetIO();

    DockSpaceUI();
    ToolbarUI();

    if (show_demo_window) {
        ImGui::ShowDemoWindow(&show_demo_window);
    }
    
    // Draw Nodes
    for (auto& node : mNodesThisFrame) {
        node->mUI->BeginFrameForViewables();
    }
    mNodesThisFrame.clear();
}

void UIManager::Render(SDL_GPUCommandBuffer* command_buffer, SDL_GPUTexture* swapchain_texture) {
        BeginFrame();
        for (auto& node : mNodesThisFrame) {
            node->mUI->BeginFrameForViewables();
        }

        // Rendering
        ImGui::Render();
        ImDrawData* draw_data = ImGui::GetDrawData();
        const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

        if (!command_buffer) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Invalid command buffer");
            return;
        }

        if (swapchain_texture != nullptr && !is_minimized)
        {
            // This is mandatory: call ImGui_ImplSDLGPU3_PrepareDrawData() to upload the vertex/index buffer!
            ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, command_buffer);

            // Setup and start a render pass
            SDL_GPUColorTargetInfo target_info = {};
            target_info.texture = swapchain_texture;
            target_info.clear_color = SDL_FColor { clear_color.x, clear_color.y, clear_color.z, clear_color.w };
            target_info.load_op = SDL_GPU_LOADOP_LOAD;
            target_info.store_op = SDL_GPU_STOREOP_STORE;
            target_info.mip_level = 1;
            target_info.layer_or_depth_plane = 0;
            target_info.cycle = false;
            std::vector<SDL_GPUColorTargetInfo> colorTargets {target_info};
            SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(command_buffer, &target_info, 1, nullptr);

            // Render ImGui
            ImGui_ImplSDLGPU3_RenderDrawData(draw_data, command_buffer, render_pass);

            SDL_EndGPURenderPass(render_pass);
        }
}

void UIManager::Shutdown() {
    ImGui_ImplSDL3_Shutdown();
    ImGui_ImplSDLGPU3_Shutdown();
    ImGui::DestroyContext();
}

void UIManager::DockSpaceUI()
{
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(ImVec2(0, toolbarSize));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x/5, viewport->Size.y - toolbarSize));
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGuiWindowFlags window_flags = 0
		| ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking
		| ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::Begin("Master DockSpace", NULL, window_flags);
	ImGuiID dockMain = ImGui::GetID("MyDockspace");
	
	ImGui::DockSpace(dockMain);
	ImGui::End();
	ImGui::PopStyleVar(3);
}

void UIManager::ToolbarUI()
{
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(ImVec2());
	ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, toolbarSize));
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGuiWindowFlags window_flags = 0
		| ImGuiWindowFlags_NoDocking 
		| ImGuiWindowFlags_NoTitleBar 
		| ImGuiWindowFlags_NoResize 
		| ImGuiWindowFlags_NoMove 
		| ImGuiWindowFlags_NoScrollbar 
		| ImGuiWindowFlags_NoSavedSettings
		;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
	ImGui::Begin("TOOLBAR", NULL, window_flags);
	ImGui::PopStyleVar();
  
	ImGui::Button("Toolbar goes here", ImVec2(0, 37));
    ImGui::SameLine();
    ImGui::Checkbox("Demo Window", &show_demo_window);
  
	ImGui::End();
}