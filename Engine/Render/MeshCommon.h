#pragma once

#include <assimp/Importer.hpp>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glm/glm.hpp>
#include <SDL3/SDL.h>
#include <string>
#include <unordered_map>
#include <vector>

class MeshManager {
public:
protected:
};

class MeshCommon {
public:
    struct Vertex {
        glm::vec3 position = {0, 0, 0};
        glm::vec2 uv = {0, 0};
    };
    struct SubMesh {
        std::vector<Vertex> vertices;
        std::vector<Uint32> indices;
        Uint32 materialIndex;
    };
public:
    MeshCommon() = default;
    ~MeshCommon() = default;

    static bool Load3DFile(const std::string filePath, MeshCommon& outScene);
    static bool LoadTextures(const aiScene* scene, MeshCommon& outMesh);
protected:
    const aiScene* mScene = nullptr;
    std::unordered_map<std::string, unsigned int*> mTextureIdMap;
};