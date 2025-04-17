#include "Renderer.h"

#ifdef VULKAN_IMPLEMENTATION
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;
#include <Volk/volk.h>
#endif

#include <assimp/Importer.hpp>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <SDL3/SDL_vulkan.h>
#include <SDL3_image/SDL_image.h>
#include <span>

// statics
static std::string BasePath;
static std::vector<std::string> SamplerNames = 
{
	"PointClamp",
	"PointWrap",
	"LinearClamp",
	"LinearWrap",
	"AnisotropicClamp",
	"AnisotropicWrap",
};
static std::vector<Renderer::ModelDescriptor> Models = 
{
    {"viking_room", "viking_room.obj", "viking_room.png"},
    {"DamagedHelmet", "DamagedHelmet.gltf", "Default_albedo.jpg"},
};

Renderer::Renderer() {}

Renderer::~Renderer() {
    Shutdown();
}

bool Renderer::Init(const char* title, int width, int height) {
    InitAssetLoader();

    mWindow = SDL_CreateWindow(title, width, height, SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE);
    if (!mWindow) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_CreateWindow failed: %s", SDL_GetError());
        return false;
    }

    mSDLDevice = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL, true, nullptr);
    if (!mSDLDevice) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_CreateGPUDevice failed: %s", SDL_GetError());
        return false;
    }
    else {
        SDL_LogDebug(SDL_LOG_CATEGORY_GPU, "GPU Driver: %s", SDL_GetGPUDeviceDriver(mSDLDevice));
    }

    if (!SDL_ClaimWindowForGPUDevice(mSDLDevice, mWindow)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_ClaimWindowForGPUDevice failed: %s", SDL_GetError());
        return false;
    }

    if (!InitPipelines()) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to initialize GPU Pipelines");
        return false;
    }

    InitSamplers();
    InitMeshes();

    SDL_ShowWindow(mWindow);

    return true;
}

void Renderer::InitAssetLoader() {
    BasePath = SDL_GetBasePath();
}

bool Renderer::InitPipelines() {
    SDL_GPUShader* vertexShader = LoadShader(mSDLDevice, "TexturedQuadWithMatrix.vert", 0, 1, 0, 0);
    if (!vertexShader) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Vertex Shader failed to load");
        return false;
    }

    SDL_GPUShader* fragmentShader = LoadShader(mSDLDevice, "TexturedQuad.frag", 1, 0, 0, 0);
    if (!fragmentShader) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Vertex Shader failed to load");
        return false;
    }

    SDL_GPUColorTargetDescription colorTargetDescription{};
    colorTargetDescription.format = SDL_GetGPUSwapchainTextureFormat(mSDLDevice, mWindow);
    std::vector colorTargetDescriptions{colorTargetDescription};
    SDL_GPUGraphicsPipelineTargetInfo pipelineTargetInfo{};
    pipelineTargetInfo.color_target_descriptions = colorTargetDescriptions.data();
    pipelineTargetInfo.num_color_targets = static_cast<Uint32>(colorTargetDescriptions.size());
    pipelineTargetInfo.has_depth_stencil_target = true;

    if (SDL_GPUTextureSupportsFormat(
        mSDLDevice, 
        SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT, 
        SDL_GPU_TEXTURETYPE_2D, 
        SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET)) {
        pipelineTargetInfo.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT;
    }
    else if (SDL_GPUTextureSupportsFormat(
        mSDLDevice, 
        SDL_GPU_TEXTUREFORMAT_D32_FLOAT_S8_UINT, 
        SDL_GPU_TEXTURETYPE_2D, 
        SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET)) {
        pipelineTargetInfo.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT_S8_UINT;
    }
    else {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "No depth-stencil format supported");
        return false;
    }
    SDL_GPUDepthStencilState depthStencilState{
        .compare_op = SDL_GPU_COMPAREOP_LESS,
        .enable_depth_test = true,
        .enable_depth_write = true,
    };

    SDL_GPUVertexBufferDescription vertexBufferDescription{};
    vertexBufferDescription.slot = 0;
    vertexBufferDescription.pitch = sizeof(PositionTextureVertex);
    vertexBufferDescription.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vertexBufferDescription.instance_step_rate = 0;
    std::vector<SDL_GPUVertexBufferDescription> vertexBufferDescriptions{vertexBufferDescription};

    SDL_GPUVertexAttribute vertexPositionAttribute{ 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 0};
    SDL_GPUVertexAttribute vertexTextureAttribute{ 1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, sizeof(glm::vec3)};
    std::vector<SDL_GPUVertexAttribute> vertexAttributes{vertexPositionAttribute, vertexTextureAttribute};
    SDL_GPUVertexInputState vertexInputState{ vertexBufferDescriptions.data(), 1, vertexAttributes.data(), 2};

    SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipelineCreateInfo.vertex_shader = vertexShader;
    pipelineCreateInfo.fragment_shader = fragmentShader;
    pipelineCreateInfo.target_info = pipelineTargetInfo;
    pipelineCreateInfo.vertex_input_state = vertexInputState;
    pipelineCreateInfo.depth_stencil_state = depthStencilState;

    pipelineCreateInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    mPipelines[RenderMode::Fill] = SDL_CreateGPUGraphicsPipeline(mSDLDevice, &pipelineCreateInfo);
    if (!mPipelines[RenderMode::Fill]) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create 'Fill' graphics pipeline");
        return false;
    }
    
	pipelineCreateInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_LINE;
    mPipelines[RenderMode::Line] = SDL_CreateGPUGraphicsPipeline(mSDLDevice, &pipelineCreateInfo);
    if (!mPipelines[RenderMode::Line]) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create 'Line' graphics pipeline");
        return false;
    }

    SDL_ReleaseGPUShader(mSDLDevice, vertexShader);
    SDL_ReleaseGPUShader(mSDLDevice, fragmentShader);

    return true;
}

void Renderer::InitSamplers() {
    // Create Samplers
	//"PointClamp",
    SDL_GPUSamplerCreateInfo pointClampSamplerCreateInfo{
        .min_filter = SDL_GPU_FILTER_NEAREST,
        .mag_filter = SDL_GPU_FILTER_NEAREST,
        .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
        .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    };
    mSamplers.emplace_back(SDL_CreateGPUSampler(mSDLDevice, &pointClampSamplerCreateInfo));
	//"PointWrap",
    SDL_GPUSamplerCreateInfo pointWrapSamplerCreateInfo{
        .min_filter = SDL_GPU_FILTER_NEAREST,
        .mag_filter = SDL_GPU_FILTER_NEAREST,
        .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
        .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
        .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
        .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
    };
    mSamplers.emplace_back(SDL_CreateGPUSampler(mSDLDevice, &pointWrapSamplerCreateInfo));
	//"LinearClamp",
    SDL_GPUSamplerCreateInfo linearClampSamplerCreateInfo{
        .min_filter = SDL_GPU_FILTER_LINEAR,
        .mag_filter = SDL_GPU_FILTER_LINEAR,
        .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
        .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    };
    mSamplers.emplace_back(SDL_CreateGPUSampler(mSDLDevice, &linearClampSamplerCreateInfo));
	//"LinearWrap",
    SDL_GPUSamplerCreateInfo linearWrapSamplerCreateInfo{
        .min_filter = SDL_GPU_FILTER_LINEAR,
        .mag_filter = SDL_GPU_FILTER_LINEAR,
        .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
        .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
        .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
        .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
    };
    mSamplers.emplace_back(SDL_CreateGPUSampler(mSDLDevice, &linearWrapSamplerCreateInfo));
	//"AnisotropicClamp",
    SDL_GPUSamplerCreateInfo anisotropicClampSamplerCreateInfo{
        .min_filter = SDL_GPU_FILTER_LINEAR,
        .mag_filter = SDL_GPU_FILTER_LINEAR,
        .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
        .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        .max_anisotropy = 4.0f,
        .enable_anisotropy = true,
    };
    mSamplers.emplace_back(SDL_CreateGPUSampler(mSDLDevice, &anisotropicClampSamplerCreateInfo));
	//"AnisotropicWrap",
    SDL_GPUSamplerCreateInfo anisotropicWrapSamplerCreateInfo{
        .min_filter = SDL_GPU_FILTER_LINEAR,
        .mag_filter = SDL_GPU_FILTER_LINEAR,
        .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
        .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
        .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
        .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
        .max_anisotropy = 4.0f,
        .enable_anisotropy = true,
    };
    mSamplers.emplace_back(SDL_CreateGPUSampler(mSDLDevice, &anisotropicWrapSamplerCreateInfo));
}

void Renderer::InitMeshes() {
    const size_t modelIdx = 1;
    const std::string modelname = Models[modelIdx].foldername;
    Mesh mesh;
    if (InitMesh(Models[modelIdx], mesh)) {
        mMeshes[modelname] = mesh;
    }
    else{
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to initialize mesh of filename: %s", modelname.c_str());
    }
}

bool Renderer::InitMesh(const ModelDescriptor& modelDescriptor, Mesh& mesh) {
    
    // Create Mesh resources
    SDL_GPUBufferCreateInfo vertexBufferCreateInfo{};
    SDL_GPUTransferBuffer* vertexTransferBuffer = nullptr;
    SDL_GPUBufferCreateInfo indexBufferCreateInfo{};
    SDL_GPUTransferBuffer* indexTransferBuffer = nullptr;
    if (!CreateModelGPUResources(modelDescriptor, mesh, vertexBufferCreateInfo, vertexTransferBuffer, indexBufferCreateInfo, indexTransferBuffer)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create model GPU resources");
        return false;
    }
    // Create Texture resources
    SDL_Surface* imageData = nullptr;
    SDL_GPUTextureCreateInfo colorTextureCreateInfo{};
    SDL_GPUTransferBuffer* textureTransferBuffer = nullptr;
    if (!CreateTextureGPUResources(modelDescriptor, mesh, imageData, colorTextureCreateInfo, textureTransferBuffer)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create texture GPU resources");
        return false;
    }

    // Upload the transfer data to the vertex buffer
    SDL_GPUCommandBuffer* uploadCmdBuff = SDL_AcquireGPUCommandBuffer(mSDLDevice);
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmdBuff);
    {   // upload vertex data
        SDL_GPUTransferBufferLocation transferBufferLocation{ vertexTransferBuffer, 0 };
        SDL_GPUBufferRegion bufferRegion{ mesh.vertexBuffer, 0, vertexBufferCreateInfo.size };
        SDL_UploadToGPUBuffer(copyPass, &transferBufferLocation, &bufferRegion, false);
    }
    {   // upload index data
        SDL_GPUTransferBufferLocation transferBufferLocation{ indexTransferBuffer, 0 };
        SDL_GPUBufferRegion bufferRegion{ mesh.indexBuffer, 0, indexBufferCreateInfo.size };
        SDL_UploadToGPUBuffer(copyPass, &transferBufferLocation, &bufferRegion, false);
    }
    {   // upload texture data
        SDL_GPUTextureTransferInfo textureTransferInfo{ .transfer_buffer = textureTransferBuffer, .offset = 0 };
        SDL_GPUTextureRegion textureRegion{ .texture = mesh.colorTexture, .w = static_cast<Uint32>(imageData->w), .h = static_cast<Uint32>(imageData->h), .d = 1 };
        SDL_UploadToGPUTexture(copyPass, &textureTransferInfo, &textureRegion, false);
    }

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(uploadCmdBuff);

    SDL_DestroySurface(imageData);
    SDL_ReleaseGPUTransferBuffer(mSDLDevice, vertexTransferBuffer);
    SDL_ReleaseGPUTransferBuffer(mSDLDevice, indexTransferBuffer);
    SDL_ReleaseGPUTransferBuffer(mSDLDevice, textureTransferBuffer);
    return true;
}

bool Renderer::CreateModelGPUResources(
    const ModelDescriptor& modelDescriptor,
    Mesh& mesh,
    SDL_GPUBufferCreateInfo& vertexBufferCreateInfo,
    SDL_GPUTransferBuffer*& vertexTransferBuffer,
    SDL_GPUBufferCreateInfo& indexBufferCreateInfo,
    SDL_GPUTransferBuffer*& indexTransferBuffer) {
    
    // Load the model
    if (!LoadModel(modelDescriptor, mesh)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load model: %s", SDL_GetError());
        return false;
    }
    // Create GPU resources
    vertexBufferCreateInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    vertexBufferCreateInfo.size = static_cast<Uint32>(mesh.vertices.size() * sizeof(PositionTextureVertex));
    mesh.vertexBuffer = SDL_CreateGPUBuffer(mSDLDevice, &vertexBufferCreateInfo);
    if (!mesh.vertexBuffer) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create 'Vertex' buffer");
        return false;
    }
    SDL_SetGPUBufferName(mSDLDevice, mesh.vertexBuffer, "Vertex Buffer");

    indexBufferCreateInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    indexBufferCreateInfo.size = static_cast<Uint32>(mesh.indices.size() * sizeof(Uint32));
    mesh.indexBuffer =  SDL_CreateGPUBuffer(mSDLDevice, &indexBufferCreateInfo);
    if (!mesh.indexBuffer) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create 'Index' buffer");
        return false;
    }
    SDL_SetGPUBufferName(mSDLDevice, mesh.indexBuffer, "Index Buffer");

    SDL_GPUTransferBufferCreateInfo vertexTransferBufferCreateInfo{};
    vertexTransferBufferCreateInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    vertexTransferBufferCreateInfo.size = vertexBufferCreateInfo.size;
    
    SDL_GPUTransferBufferCreateInfo indexTransferBufferCreateInfo{};
    indexTransferBufferCreateInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    indexTransferBufferCreateInfo.size = indexBufferCreateInfo.size;

    vertexTransferBuffer = SDL_CreateGPUTransferBuffer(mSDLDevice, &vertexTransferBufferCreateInfo);
    if (!vertexTransferBuffer) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create GPU vertex transfer buffer");
        return false;
    }
    
    indexTransferBuffer = SDL_CreateGPUTransferBuffer(mSDLDevice, &indexTransferBufferCreateInfo);
    if (!indexTransferBuffer) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create GPU index transfer buffer");
        return false;
    }

    void* vertexBufferDataPtr = SDL_MapGPUTransferBuffer(mSDLDevice, vertexTransferBuffer, false);
    void* indexBufferDataPtr = SDL_MapGPUTransferBuffer(mSDLDevice, indexTransferBuffer, false);
    if (!vertexBufferDataPtr || !indexBufferDataPtr) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to map GPU transfer buffer data pointers");
        return false;
    }
    else {
        std::span transferBufferData{ static_cast<PositionTextureVertex*>(vertexBufferDataPtr), mesh.vertices.size()};
        std::ranges::copy(mesh.vertices, transferBufferData.begin());

        std::span indexBufferData{ static_cast<Uint32*>(indexBufferDataPtr), mesh.indices.size()};
        std::ranges::copy(mesh.indices, indexBufferData.begin());
    }

    SDL_UnmapGPUTransferBuffer(mSDLDevice, vertexTransferBuffer);
    SDL_UnmapGPUTransferBuffer(mSDLDevice, indexTransferBuffer);
    return true;
}


bool Renderer::CreateTextureGPUResources(
    const ModelDescriptor& modelDescriptor,
    Mesh& mesh,
    SDL_Surface*& imageData,
    SDL_GPUTextureCreateInfo& textureCreateInfo,
    SDL_GPUTransferBuffer*& textureTransferBuffer) {

    // Load the image
    imageData = LoadImage(modelDescriptor, 4);
    if (!imageData) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load image: %s", SDL_GetError());
        return false;
    }

    textureCreateInfo = SDL_GPUTextureCreateInfo{
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
        .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = static_cast<Uint32>(imageData->w),
        .height = static_cast<Uint32>(imageData->h),
        .layer_count_or_depth = 1,
        .num_levels = 1,
    };
    mesh.colorTexture = SDL_CreateGPUTexture(mSDLDevice, &textureCreateInfo);
    if (!mesh.colorTexture) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create texture: %s", SDL_GetError());
        return false;
    }
    SDL_SetGPUTextureName(mSDLDevice, mesh.colorTexture, modelDescriptor.textureFilename.c_str());

    Resize(); // Setup the Depth Texture
    
    // Set the texture data
    SDL_GPUTransferBufferCreateInfo textureTransferBufferCreateInfo{
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = static_cast<Uint32>(imageData->h * imageData->w * 4),
    };
    textureTransferBuffer = SDL_CreateGPUTransferBuffer(mSDLDevice, &textureTransferBufferCreateInfo);
    void* textureDataPtr = static_cast<Uint8*>(SDL_MapGPUTransferBuffer(mSDLDevice, textureTransferBuffer, false));
    if (!textureDataPtr) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to map GPU transfer buffer data pointer for texture");
        return false;
    }
    else {
        SDL_memcpy(textureDataPtr, imageData->pixels, textureTransferBufferCreateInfo.size);
    }
    SDL_UnmapGPUTransferBuffer(mSDLDevice, textureTransferBuffer);
    
    return true;
}

std::string Renderer::GetFolderName(const std::string& filename) const {
    auto it = std::find_if(Models.begin(), Models.end(), [&filename](const ModelDescriptor& model) {
        return model.meshFilename == filename || model.textureFilename == filename;
    });
    if (it != Models.end()) {
        return it->foldername;
    }
    return std::string{};
}

SDL_GPUShader* Renderer::LoadShader(
    SDL_GPUDevice* device,
    const std::string& shaderFilename,
    const Uint32 samplerCount,
    const Uint32 uniformBufferCount,
    const Uint32 storageBufferCount,
    const Uint32 storageTextureCount) {

    // Auto-detect the shader stage from the file name for convenience
    SDL_GPUShaderStage stage;
    if (shaderFilename.contains(".vert"))
    {
        stage = SDL_GPU_SHADERSTAGE_VERTEX;
    }
    else if (shaderFilename.contains(".frag"))
    {
        stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    }
    else
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Invalid shader stage!");
        return nullptr;
    }

	std::string fullPath;
	SDL_GPUShaderFormat backendFormats = SDL_GetGPUShaderFormats(device);
	SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
	const char *entrypoint;

	if (backendFormats & SDL_GPU_SHADERFORMAT_SPIRV) {
        fullPath = std::format("{}/Content/Shaders/Compiled/SPIRV/{}.spv", BasePath, shaderFilename);
		format = SDL_GPU_SHADERFORMAT_SPIRV;
		entrypoint = "main";
	} else if (backendFormats & SDL_GPU_SHADERFORMAT_MSL) {
        fullPath = std::format("{}/Content/Shaders/Compiled/MSL/{}.msl", BasePath, shaderFilename);
		format = SDL_GPU_SHADERFORMAT_MSL;
		entrypoint = "main0";
	} else if (backendFormats & SDL_GPU_SHADERFORMAT_DXIL) {
        fullPath = std::format("{}/Content/Shaders/Compiled/DXIL/{}.dxil", BasePath, shaderFilename);
		format = SDL_GPU_SHADERFORMAT_DXIL;
		entrypoint = "main";
	} else {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s", "Unrecognized backend shader format!");
		return nullptr;
	}

    std::ifstream file{fullPath, std::ios::binary};
    if (!file) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Couldn't open shader file: %s", fullPath);
        return nullptr;
    }
    std::vector<Uint8> code{std::istreambuf_iterator(file), {}};

	SDL_GPUShaderCreateInfo shaderInfo{};
    shaderInfo.code = code.data();
    shaderInfo.code_size = code.size();
    shaderInfo.entrypoint = entrypoint;
    shaderInfo.format = format;
    shaderInfo.stage = stage;
    shaderInfo.num_samplers = samplerCount;
    shaderInfo.num_uniform_buffers = uniformBufferCount;
    shaderInfo.num_storage_buffers = storageBufferCount;
    shaderInfo.num_storage_textures = storageTextureCount;

	SDL_GPUShader* shader = SDL_CreateGPUShader(device, &shaderInfo);
	if (shader == nullptr)
	{
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create shader!");
		return nullptr;
	}

	return shader;
}

SDL_Surface* Renderer::LoadImage(const ModelDescriptor& modelDescriptor, int desiredChannels) {
    // Construct the full path
    std::string fullPath = std::format("{}/Content/Models/{}/{}", BasePath, modelDescriptor.foldername, modelDescriptor.textureFilename);
    SDL_Surface* image = IMG_Load(fullPath.c_str());
    SDL_PixelFormat format = SDL_PIXELFORMAT_UNKNOWN;
    if (!image) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load image: %s", SDL_GetError());
        return nullptr;
    }
    if (desiredChannels == 4) {
        format = SDL_PIXELFORMAT_ABGR8888;
    }
    else {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unexpected number of channels: %d", desiredChannels);
        SDL_DestroySurface(image);
        return nullptr;
    }
    if (image->format != format) {
        SDL_Surface* convertedImage = SDL_ConvertSurface(image, format);
        if (!convertedImage) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to convert image format: %s", SDL_GetError());
            SDL_DestroySurface(image);
            return nullptr;
        }
        SDL_DestroySurface(image);
        image = convertedImage;
    }
    return image;
}

bool Renderer::LoadModel(const ModelDescriptor& modelDescriptor, Renderer::Mesh & outMesh) {
    // Construct the full path
    std::string modelFullPath = std::format("{}/Content/Models/{}/{}", BasePath, modelDescriptor.foldername, modelDescriptor.meshFilename);
    // Load the model
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(modelFullPath, aiProcess_Triangulate | aiProcess_FlipUVs);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->HasMeshes()) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load model: %s \nmodel filepath: %s", importer.GetErrorString(), modelFullPath.c_str());
        return false;
    }
    for (size_t i = 0; i < scene->mNumMeshes; ++i) {
        auto mesh = scene->mMeshes[i];
        if (mesh->HasPositions()) {
            outMesh.vertices.reserve(mesh->mNumVertices);
            for (size_t j = 0; j < mesh->mNumVertices; ++j) {
                auto vertex = mesh->mVertices[j];
                auto uv = mesh->mTextureCoords[0][j];
                outMesh.vertices.push_back({ 
                    .position = {
                        vertex.x, 
                        vertex.y, 
                        vertex.z
                    }, 
                    .uv = {
                        uv.x, 
                        uv.y
                    }
                });
            }
            for (size_t j = 0; j < mesh->mNumFaces; ++j) {
                auto face = mesh->mFaces[j];
                outMesh.indices.reserve(face.mNumIndices);
                for (size_t k = 0; k < face.mNumIndices; ++k) {
                    outMesh.indices.push_back(face.mIndices[k]);
                }
            }
        }
    }
    return true;
}

bool Renderer::GPURenderPass(SDL_Window* window) {
    SDL_GPUCommandBuffer* commandBuffer(SDL_AcquireGPUCommandBuffer(mSDLDevice));
    if (!commandBuffer) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_AcquireGPUCommandBuffer failed: %s", SDL_GetError());
        return false;
    }

    SDL_GPUTexture* swapchainTexture;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, window, &swapchainTexture, nullptr, nullptr)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_WaitAndAcquireGPUSwapchainTexture failed: %s", SDL_GetError());
        return false;
    }
    if (swapchainTexture) {
        SDL_GPUColorTargetInfo colorTarget{};
        colorTarget.texture = swapchainTexture;
        colorTarget.store_op = SDL_GPU_STOREOP_STORE;
        colorTarget.load_op = SDL_GPU_LOADOP_CLEAR;
        colorTarget.clear_color = SDL_FColor{0.3f,0.2f,0.2f,1.0f};
        std::vector<SDL_GPUColorTargetInfo> colorTargets {colorTarget};

        SDL_GPUDepthStencilTargetInfo depthStencilTarget{};
        depthStencilTarget.texture = mDepthTexture;
        depthStencilTarget.clear_depth = 1.0f;
        depthStencilTarget.load_op = SDL_GPU_LOADOP_CLEAR;
        depthStencilTarget.store_op = SDL_GPU_STOREOP_STORE;
        depthStencilTarget.clear_stencil = 0;
        
        SDL_GPURenderPass* renderPass{
            SDL_BeginGPURenderPass(commandBuffer, colorTargets.data(), static_cast<Uint32>(colorTargets.size()), &depthStencilTarget)
        };

        for (auto& namedMesh : mMeshes) {
            Mesh& mesh = namedMesh.second;

            SDL_BindGPUGraphicsPipeline(renderPass, mPipelines[mRenderMode]);
            std::vector<SDL_GPUBufferBinding> bindings{{mesh.vertexBuffer, 0}};
            SDL_BindGPUVertexBuffers(renderPass, 0, bindings.data(), static_cast<Uint32>(bindings.size()));
            SDL_GPUBufferBinding indexBufferBinding{mesh.indexBuffer, 0};
            SDL_BindGPUIndexBuffer(renderPass, &indexBufferBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
            SDL_GPUTextureSamplerBinding textureSamplerBinding{mesh.colorTexture, mSamplers[mCurrentSamplerIndex]};
            SDL_BindGPUFragmentSamplers(renderPass, 0, &textureSamplerBinding, 1);
    
            // projection matrix
            const float fovY = glm::radians(90.0f/mCachedScreenAspectRatio);
            glm::mat4 projectionMatrix = glm::perspective(fovY, mCachedScreenAspectRatio, 0.1f, 100.0f);
            // view matrix
            glm::mat4 viewMatrix = glm::lookAt(glm::vec3(0, -3, 2), glm::vec3(0, 0, 1), glm::vec3(0, 0, 1));
            // model matrix
            glm::mat4 modelMatrix = glm::mat4(1.0f);
            modelMatrix = glm::scale(modelMatrix, glm::vec3(mScale));
            modelMatrix = glm::rotate(modelMatrix, SDL_GetTicks() / 1000.0f, glm::vec3(0, 0, 1));
            modelMatrix = glm::translate(modelMatrix, glm::vec3(0, 0, 0));
            // modelviewprojection matrix
            glm::mat4 mvpMatrix = (projectionMatrix * viewMatrix) * modelMatrix;
            SDL_PushGPUVertexUniformData(commandBuffer, 0, &mvpMatrix, sizeof(glm::mat4));
    
            SDL_DrawGPUIndexedPrimitives(renderPass, static_cast<Uint32>(mesh.indices.size()), 1, 0, 0, 0);
        }
        SDL_EndGPURenderPass(renderPass);
    }

    if (!SDL_SubmitGPUCommandBuffer(commandBuffer)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_SubmitGPUCommandBuffer failed: %s", SDL_GetError());
        return false;
    }
    return true;
}

void Renderer::Clear() {
}

void Renderer::Present() {
    GPURenderPass(mWindow);
}

void Renderer::Shutdown() {
    for (auto modePipeline : mPipelines) {
        SDL_ReleaseGPUGraphicsPipeline(mSDLDevice, modePipeline.second);
    }
    for (auto sampler : mSamplers) {
        SDL_ReleaseGPUSampler(mSDLDevice, sampler);
    }
    
    for (auto& namedMesh : mMeshes) {
        Mesh& mesh = namedMesh.second;
        if (mesh.vertexBuffer) SDL_ReleaseGPUBuffer(mSDLDevice, mesh.vertexBuffer);
        if (mesh.indexBuffer) SDL_ReleaseGPUBuffer(mSDLDevice, mesh.indexBuffer);
        if (mesh.colorTexture) SDL_ReleaseGPUTexture(mSDLDevice, mesh.colorTexture);
    }
    if (mDepthTexture) SDL_ReleaseGPUTexture(mSDLDevice, mDepthTexture);
    if (mWindow) SDL_DestroyWindow(mWindow);
}

void Renderer::Resize() {
    if (mDepthTexture) SDL_ReleaseGPUTexture(mSDLDevice, mDepthTexture);
    int windowWidth, windowHeight;
    if (!SDL_GetWindowSize(mWindow, &windowWidth, &windowHeight)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_GetWindowSize failed: %s", SDL_GetError());
        return;
    }
    SDL_GPUTextureCreateInfo depthTextureCreateInfo{
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT,
        .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
        .width = static_cast<Uint32>(windowWidth),
        .height = static_cast<Uint32>(windowHeight),
        .layer_count_or_depth = 1,
        .num_levels = 1,
        .sample_count = SDL_GPU_SAMPLECOUNT_1,
    };
    mDepthTexture = SDL_CreateGPUTexture(mSDLDevice, &depthTextureCreateInfo);
    SDL_SetGPUTextureName(mSDLDevice, mDepthTexture, "Depth Texture");

    mCachedScreenAspectRatio = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);
}

void Renderer::CycleRenderMode() {
    Uint8 value = static_cast<Uint8>(mRenderMode);
    mRenderMode = static_cast<RenderMode>(++value);
    if (mRenderMode == RenderMode::count) {
        mRenderMode = RenderMode::Fill;
    }
}

void Renderer::CycleSampler() {
    mCurrentSamplerIndex = ++mCurrentSamplerIndex;
    if (mCurrentSamplerIndex > mSamplers.size() - 1) {
        mCurrentSamplerIndex = 0;
    }
}

void Renderer::IncreaseScale() {
    mScale += mScaleStep;
    if (mScale > 5.0f) {
        mScale = 5.0f;
    }
}

void Renderer::DecreaseScale() {
    mScale -= mScaleStep;
    if (mScale < 0.2f) {
        mScale = 0.2f;
    }
}

/* To Init Vulkan, the following are needed:
    1. Instance: the Vulkan API Context. This is where we enable extensions for things
        like validation layers, logging, and more. It is the root/global context of the app.
    2. PhysicalDevice: enumerate the hardware for all 'GPUs', and query their features.
    3. Device: the driver on the GPU. Extensions can be enabled for more functionality at
        the cost of speed. We can create multiple devices for 'multi-GPU' support
    4. Swapchain: this is a list of 'images' that are needed to send 'images' to the screen.
        The number of 'images' in the Swapchain is what allows double or triple buffering.
        The Swapchain must be rebuilt if the image changes size or format.
*/
#ifdef VULKAN_IMPLEMENTATION
void Renderer::InitVulkan(SDL_Window* window) {
    SDL_Vulkan_LoadLibrary(nullptr);
    volkInitializeCustom((PFN_vkGetInstanceProcAddr)SDL_Vulkan_GetVkGetInstanceProcAddr());

    // Get required extensions from SDL
    uint32_t extCount = 0;
    auto requiredExtensions = SDL_Vulkan_GetInstanceExtensions(&extCount);

    vk::ApplicationInfo appInfo;
    appInfo.pApplicationName = "SandCastleGame";
    appInfo.pEngineName = "SandCastleEngine";
    appInfo.apiVersion = VK_API_VERSION_1_2;

    vk::InstanceCreateInfo createInfo;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = extCount;
    createInfo.ppEnabledExtensionNames = requiredExtensions;
    createInfo.flags = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;

    mInstance = vk::createInstance(createInfo);
    mDispatchLoader.init(mInstance);
    volkLoadInstanceOnly(mInstance);

    // We're assuming the first Physical Device is the one we want
    // But we probably want to make sure we're using the dGPU and not the iGPU
    mPhysicalDevice = mInstance.enumeratePhysicalDevices().front();


}
#endif
