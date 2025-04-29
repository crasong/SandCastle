#include "Components.h"

#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <Entity.h>
#include <imgui.h>
#include <queue>
#include <stack>
#include <string>

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
        ImGui::Checkbox("Show", &mShow);
        if (ImGui::TreeNode("aiScene")) {
            DisplaySceneDetails();
            ImGui::TreePop();
        }
    }
}
void DisplayComponent::DisplaySceneDetails() {
    if (!scene) {
        scene = importer.ReadFile(mMesh->filepath, aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_FlipUVs);
        
        rootUI.node = scene->mRootNode;
        std::queue<UINode> queue;
        queue.push(rootUI);
        while (!queue.empty()) {
            UINode& curr = queue.front();
            queue.pop();
            for (size_t i = 0; i < curr.node->mNumChildren; ++i) {
                UINode child;
                child.node = curr.node->mChildren[i];
                curr.node->mTransformation;
                curr.children.push_back(child);
                queue.push(child);
            }
            //curr.children.size();
        }
    }

    const bool bIsBinary = scene->mNumTextures > 0;
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar
                                    | ImGuiWindowFlags_AlwaysVerticalScrollbar;
    ImGuiChildFlags child_flags = ImGuiChildFlags_Border;

    ImGui::BeginChild("Scene Details", ImVec2(ImGui::GetContentRegionAvail().x, 300), child_flags, window_flags);
    if (ImGui::TreeNode("meshes")) {
        for (size_t i = 0; i < scene->mNumMeshes; ++i) {
            auto mesh = scene->mMeshes[i];
            if (mesh){
                ImGui::Text("name: %s, materialIdx: %i, verts: %i"
                    , mesh->mName.C_Str(), mesh->mMaterialIndex, mesh->mNumVertices);
            }
            else {
                ImGui::Text("NULL");
            }
        }
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("materials")) {
        for (size_t i = 0; i < scene->mNumMaterials; ++i) {
            auto material = scene->mMaterials[i];
            if (material) {
                ImGui::Text("name: %s", material->GetName().C_Str());
                for (size_t j = 0; j < AI_TEXTURE_TYPE_MAX; ++j) {
                    const aiTextureType type = (aiTextureType)j;
                    const uint8_t texCount = material->GetTextureCount(type);
                    for (size_t k = 0; k < texCount; ++k){
                        aiString texturePath;
                        if (material->GetTexture(type, k, &texturePath) == AI_SUCCESS) {
                            //std::string pathStr(texturePath.C_Str());
                            //pathStr = pathStr.substr(1, 1);
                            //int texIdx = std::stoi(pathStr);
                            ImGui::Text("\t%s\t: [%i] %s", aiTextureTypeToString(type), k, texturePath.C_Str());
                            if (bIsBinary) {
                                const aiTexture* texture = scene->GetEmbeddedTexture(texturePath.C_Str());
                                ImGui::Text("\t\tfilename: %s %s [%i, %i]", texture->mFilename.C_Str(), texture->achFormatHint, texture->mWidth, texture->mHeight);
                            }
                        }
                    }
                }
            }
            else {
                ImGui::Text("NULL");
            }
        }
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("textures")) {
        for (size_t i = 0; i < scene->mNumTextures; ++i) {
            auto texture = scene->mTextures[i];
            if (texture) {
                ImGui::Text("filename: %s [%i, %i]", texture->mFilename.C_Str(), texture->mWidth, texture->mHeight);
            }
            else {
                ImGui::Text("NULL");
            }
        }
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Nodes")) {
        ImGui::TreePop();
    }
    ImGui::EndChild();
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
        ImGui::InputFloat3("Center", &mCenter.x);
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