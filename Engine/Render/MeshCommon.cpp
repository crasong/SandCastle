#include "MeshCommon.h"

// defines
#define MODEL_PATH "Content/Models/"
#define IMAGE_PATH "Content/Images/"
#define SHADER_PATH "Content/Shaders/Compiled/"
#define EMPTY_PATH ""
#define GLTF_PATH "glTF"
#define GLTF_EXT ".gltf"
#define GLTF_EMBEDDED_PATH "glTF-Embedded"
#define GLTF_EMBEDDED_EXT ".gltf"
#define GLTF_BINARY_PATH "glTF-Binary"
#define GLTF_BINARY_EXT ".glb"
#define OBJ_EXT ".obj"

bool MeshCommon::Load3DFile(const std::string filePath, MeshCommon& outMesh) {
    // Load the model
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filePath, aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_FlipUVs);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->HasMeshes()) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load model: %s \nmodel filepath: %s", importer.GetErrorString(), filePath.c_str());
        return false;
    }
    const float xMod = 1.0f; //modelDescriptor.flipX ? -1.0f : 1.0f;
    const float yMod = 1.0f; //modelDescriptor.flipY ? -1.0f : 1.0f;
    const float zMod = 1.0f; //modelDescriptor.flipZ ? -1.0f : 1.0f;
    std::vector<SubMesh> subMeshes;
    for (size_t i = 0; i < scene->mNumMeshes; ++i) {
        SubMesh subMesh{};
        auto mesh = scene->mMeshes[i];
        if (mesh->HasPositions()) {
            subMesh.vertices.reserve(mesh->mNumVertices);
            for (size_t j = 0; j < mesh->mNumVertices; ++j) {
                auto vertex = mesh->mVertices[j];
                auto uv = mesh->mTextureCoords[0][j];
                subMesh.vertices.push_back({ 
                    .position = {
                        vertex.x * xMod,
                        vertex.y * yMod,
                        vertex.z * zMod
                    }, 
                    .uv = {
                        uv.x, 
                        uv.y
                    }
                });
            }
            for (size_t j = 0; j < mesh->mNumFaces; ++j) {
                auto face = mesh->mFaces[j];
                subMesh.indices.reserve(face.mNumIndices);
                for (size_t k = 0; k < face.mNumIndices; ++k) {
                    subMesh.indices.push_back(face.mIndices[k]);
                }
            }
            subMesh.materialIndex = mesh->mMaterialIndex;
            subMeshes.push_back(subMesh);
        }
    }
    // For each material, build a map of texture types to texture indices
    std::vector<std::unordered_map<aiTextureType, std::vector<Uint32>>> materialTexTypeIndexMap;
    for (size_t i = 0; i < scene->mNumMaterials; ++i) {
        auto material = scene->mMaterials[i];
        if (material) {
            std::unordered_map<aiTextureType, std::vector<Uint32>> texTypeIndexMap;
            for (size_t j = 0; j < AI_TEXTURE_TYPE_MAX; ++j) {
                std::vector<Uint32> texIndices;
                const aiTextureType type = (aiTextureType)j;
                const uint8_t texCount = material->GetTextureCount(type);
                for (unsigned int k = 0; k < texCount; ++k){
                    aiString texturePath;
                    if (material->GetTexture(type, k, &texturePath) == AI_SUCCESS) {
                        std::string pathStr(texturePath.C_Str());
                        pathStr = pathStr.substr(1, 1);
                        texIndices.push_back(std::stoi(pathStr));
                    }
                    //ImGui::Text("\t%s\t: [%i] %s", aiTextureTypeToString(type), k, texturePath.C_Str());
                }
                if (texIndices.size() > 0) {
                    texTypeIndexMap[type] = texIndices;
                }
            }
            materialTexTypeIndexMap.push_back(texTypeIndexMap);
        }
    }
    // For each texture, load them?
    for (size_t i = 0; i < scene->mNumTextures; ++i) {
        auto texture = scene->mTextures[i];
        
    }
    // outMesh.samplerTypeIndex = modelDescriptor.samplerTypeIndex;
    outMesh.mScene = scene;
    return true;
}

bool MeshCommon::LoadTextures(const aiScene* scene, MeshCommon& outMesh) {
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->HasMaterials()) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load textures: %s", scene->mName.C_Str());
        return false;
    }
    outMesh.mTextureIdMap.clear();

    for (size_t i = 0; i < scene->mNumMaterials; ++i) {
		int texIndex = 0;
		aiReturn texFound = AI_SUCCESS;

		aiString path;	// filename
		while (texFound == AI_SUCCESS)
		{
			texFound = scene->mMaterials[i]->GetTexture(aiTextureType_DIFFUSE, texIndex, &path);
			outMesh.mTextureIdMap[path.data] = nullptr; //fill map with textures, pointers still NULL yet
			texIndex++;
		}
    }

    const size_t numTextures = outMesh.mTextureIdMap.size();
    auto itr = outMesh.mTextureIdMap.begin();
    for (size_t i = 0; i < numTextures; ++i) {
        std::string filename = (*itr).first;
    }
    
    return true;
}