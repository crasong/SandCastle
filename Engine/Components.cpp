#include "Components.h"

#include <Entity.h>
#include <imgui.h>

void TransformComponent::BeginFrame() {
    // Display position, rotation, and scale in the imgui UI as editable fields
    ImGui::Text("Transform Component");
    ImGui::InputFloat3("Position", &mPosition.x);
    ImGui::InputFloat3("Rotation", &mRotation.x);
    ImGui::InputFloat3("Scale", &mScale.x);

}
void VelocityComponent::BeginFrame() {
    // Update logic for the velocity component can be added here if needed
    ImGui::Text("Velocity Component");
    ImGui::InputFloat3("Velocity", &mVelocity.x);
    ImGui::InputFloat3("Angular Velocity", &mAngularVelocity.x);
}
void DisplayComponent::BeginFrame() {
    // Display the mesh information in the imgui UI
    if (mMesh) {
        ImGui::Text("Display Component");
        ImGui::Text("Vertices: %i", mMesh->vertices.size());
        ImGui::Text("Indices: %i", mMesh->indices.size());
    }
}

void UIComponent::BeginFrame() {
    // Update logic for the UI component can be added here if needed
    std::vector<IUIViewable*> uiViewables;
    if (mEntity) {
        mEntity->GetComponents(uiViewables); // Call the function to get viewables
    }
    ImGui::BeginGroup();
    ImGui::Text("UI Component");
    // for (auto& ui : uiViewables) {
    //     if (ui) {
    //         ImGui::Text("UI Component");
    //     }
    // }
    ImGui::EndGroup();
}

void UIComponent::BeginFrameForViewables() {
    // This function is called to begin the frame for all viewable components
    std::vector<IUIViewable*> uiViewables;
    if (mEntity) {
        mEntity->GetComponents(uiViewables, true); // Call the function to get viewables
    }
    for (auto& ui : uiViewables) {
        if (ui) {
            ui->BeginFrame();
        }
    }
}