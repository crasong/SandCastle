#pragma once

#include <assimp/material.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <SDL3/SDL_gpu.h>
#include <stack>

#define IDENTITY_MATRIX	  glm::mat4(1.0f, 0.0f, 0.0f, 0.0f, \
									0.0f, 1.0f, 0.0f, 0.0f, \
									0.0f, 0.0f, 1.0f, 0.0f, \
									0.0f, 0.0f, 0.0f, 1.0f);\

static const std::vector<aiTextureType> s_TextureTypes = {
    // These types work for DamagedHelmet
    aiTextureType_BASE_COLOR,
    aiTextureType_NORMALS,
    //aiTextureType_EMISSIVE,
    //aiTextureType_METALNESS,
    //aiTextureType_DIFFUSE_ROUGHNESS,
    //aiTextureType_LIGHTMAP,
};

struct SunLight {
	glm::vec3 direction = {-1.0f, -1.0f, -1.0f};
	glm::vec3 color 	= { 1.0f,  1.0f,  1.0f};
	bool enabled = false;
};

struct PointLight {
	glm::vec3 position = {0.0f, 0.0f, 0.0f};
	glm::vec3 color    = {1.0f, 1.0f, 1.0f};
	bool enabled = false;
};

struct SceneLighting {
	glm::vec3 ambientLight;
	PointLight pointLights[8];
	bool inited = false;
};

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec3 tangent;
	glm::vec3 bitangent;
	glm::vec2 uv;
};

struct Texture {
	Texture() {}
	Texture(aiTextureType type) { type = type; }
	Texture(SDL_GPUTexture* texture, SDL_GPUSampler* sampler) {
		texture = texture;
		sampler = sampler;
	}
	Texture(const Texture& other) {
		type = other.type;
		texture = other.texture;
		sampler = other.sampler;
	}
	aiTextureType type = aiTextureType_NONE;
	SDL_GPUTexture* texture = nullptr;
	SDL_GPUSampler* sampler = nullptr;
};

struct PBRMaterial {
	bool isValid    = false;
	std::unordered_map<aiTextureType, Texture> textureMap;
};

struct MeshEntry {
	uint32_t baseVertex  = 0;
	uint32_t baseIndex 	 = 0;
	uint32_t numVertices = 0;
	uint32_t numIndices  = 0;
	uint32_t validFaces  = 0;
	int nodeId 	 		 = 0;
	int materialIndex    = -1;
	glm::mat4 transformation; // cached & pre-transformed
};

// TODO: Put these elsewhere, like a SceneManager
struct SceneNode {
	SceneNode() {}
	SceneNode(int parentId, int id) {
		parentId = parentId;
		id = id;
	}
	glm::mat4 transformation = glm::mat4(1.0f);
	int parentId = -1;
	int id = 0;
	std::vector<int> childIds;
};

struct Mesh {
	std::vector<Vertex> vertices;
	std::vector<Uint32> indices;
	uint8_t samplerTypeIndex = 0;
	SDL_GPUBuffer* vertexBuffer = nullptr;
	SDL_GPUBuffer* indexBuffer = nullptr;
	std::unordered_map<std::string, Texture> textureIdMap;
	std::vector<MeshEntry> submeshes;
	std::vector<PBRMaterial> materials;
	std::unordered_map<uint32_t, SceneNode> nodeMap;
	std::string filepath;
	glm::mat4 globalTransform;
	bool bDoNotRender = false;
};

// mirrors camera buffer on GPU
struct CameraGPU
{
	glm::mat4 view 			 = {1.0f};
	glm::mat4 projection 	 = {1.0f};
	glm::mat4 viewProjection = {1.0f};
	glm::vec3 viewPosition   = {0.0f, 0.0f, 0.0f};
};

//parameters for grid rendering
struct GridParamsVertGPU
{
	glm::mat4 model;
};

//parameters for grid rendering
struct GridParamsFragGPU
{
	glm::vec2 offset;
	uint32_t numCells;
	float thickness;
	float scroll;
};

static void UpdateCachedTransformations(Mesh& mesh) {
	for (MeshEntry& submesh : mesh.submeshes) {
		SceneNode& node = mesh.nodeMap[submesh.nodeId];
		glm::mat4 worldTransform = node.transformation;
		int nextId = node.parentId;
		while (nextId > -1) {
			worldTransform *= mesh.nodeMap[nextId].transformation;
			nextId = mesh.nodeMap[nextId].parentId;
		}
		submesh.transformation = worldTransform;
	}
}

static void GetValidTextureBindings(const PBRMaterial& material, std::vector<SDL_GPUTextureSamplerBinding>& outBindings) {
	// for (auto& [type, texture] : material.textureMap) {
	// 	outBindings.push_back({texture.texture, texture.sampler});
	// }
	for (auto type : s_TextureTypes) {
		const Texture& texture = material.textureMap.at(type);
		outBindings.push_back({texture.texture, texture.sampler});
	}
	SDL_assert(outBindings.size() == material.textureMap.size());
}