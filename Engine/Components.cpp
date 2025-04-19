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
        ImGui::Text("Mesh Information");
        ImGui::Text("\tVertices: %i", mMesh->vertices.size());
        ImGui::Text("\tIndices: %i", mMesh->indices.size());
    }
}

void UIComponent::BeginFrame() {
    // Update logic for the UI component can be added here if needed
    std::vector<IUIViewable*> uiViewables;
    if (mEntity) {
        mEntity->GetComponents(uiViewables); // Call the function to get viewables
    }
    //ImGui::BeginGroup();
    //ImGui::Text("UI Component");
    // for (auto& ui : uiViewables) {
    //     if (ui) {
    //         ImGui::Text("UI Component");
    //     }
    // }
    //ImGui::EndGroup();
}

void CameraComponent::BeginFrame() {
    // Update logic for the camera component can be added here if needed
    ImGui::Text("Camera Component");
    // Add camera-specific UI elements here
    ImGui::InputFloat("Near Plane", &mNearPlane);
    ImGui::SameLine();
    ImGui::InputFloat("Far Plane", &mFarPlane);
    if (ImGui::Button("Perspective")) {
        mProjectionMode = Renderer::ProjectionMode::Perspective;
    }
    ImGui::SameLine();
    if (ImGui::Button("Orthographic")) {
        mProjectionMode = Renderer::ProjectionMode::Orthographic;
    }
    ImGui::SameLine();
    ImGui::Text("Projection Mode: %s", mProjectionMode == Renderer::ProjectionMode::Perspective ? "Perspective" : "Orthographic");
    if (mProjectionMode == Renderer::ProjectionMode::Perspective) {
        ImGui::InputFloat("Field of View", &mFOV);
    }
    else if (mProjectionMode == Renderer::ProjectionMode::Orthographic) {
        ImGui::InputFloat("Ortho Size", &mOrthoSize);
    }

    if (ImGui::Button("First Person")) {
        mCameraMode = CameraComponent::CameraMode::FirstPerson;
    }
    ImGui::SameLine();
    if (ImGui::Button("Third Person")) {
        mCameraMode = CameraComponent::CameraMode::ThirdPerson;
    }
    ImGui::SameLine();
    ImGui::Text("Camera Mode is %s", mCameraMode == CameraComponent::CameraMode::FirstPerson ? "First Person" : "Third Person");
    if (mCameraMode == CameraComponent::CameraMode::ThirdPerson) {
        ImGui::InputFloat3("Look At", &mLookAt.x);
        ImGui::SameLine();
        ImGui::InputFloat3("Up Vector", &mUp.x);
    }
    // select projection mode
}

void UIComponent::BeginFrameForViewables() {
    // This function is called to begin the frame for all viewable components
    std::string name;
    std::vector<IUIViewable*> uiViewables;
    if (mEntity) {
        name = mEntity->GetName();
        mEntity->GetComponents(uiViewables, true); // Call the function to get viewables
    }
	ImGui::Begin(name.c_str(), NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);
    for (auto& ui : uiViewables) {
        if (ui) {
            ImGui::PushID(ui);
            ui->BeginFrame();
            ImGui::PopID();
        }
    }
    ImGui::End();
}