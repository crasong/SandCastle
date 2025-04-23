#pragma once
#include <glm/glm.hpp>

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
