#pragma once
#include <glm/glm.hpp>

struct Light {
	glm::vec3 direction;
	glm::vec3 radiance;
	bool enabled = false;
};

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec3 tangent;
	glm::vec3 bitangent;
	glm::vec2 uv;
};

struct PositionTextureVertex {
	glm::vec3 position = {0, 0, 0};
	glm::vec2 uv = {0, 0};
};

struct MeshTexture {
	SDL_GPUTexture* texture = nullptr;
	aiTextureType type = aiTextureType_NONE;
};

struct Mesh {
	glm::vec3 mRootPosition = {0.0f, 0.0f, 0.0f}; // Base position in local space
	glm::vec3 mRootOrientation = {0.0f, 0.0f, 0.0f}; // Base orientation in degrees
	glm::vec3 mRootScale = {1.0f, 1.0f, 1.0f};
	std::vector<PositionTextureVertex> vertices;
	std::vector<Uint32> indices;
	uint8_t samplerTypeIndex = 0;
	SDL_GPUBuffer* vertexBuffer = nullptr;
	SDL_GPUBuffer* indexBuffer = nullptr;
	//std::unordered_map<aiTextureType, SDL_GPUTexture*> textureMap;
	std::unordered_map<std::string, MeshTexture> textureIdMap;
	std::string filepath;
	bool bDoNotRender = false;
};

// mirrors camera buffer on GPU
struct CameraGPU
{
	glm::mat4 view = {1.0f};
	glm::mat4 projection = {1.0f};
	glm::mat4 viewProjection = {1.0f};
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
