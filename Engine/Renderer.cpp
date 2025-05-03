#include "Renderer.h"

#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <filesystem>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlgpu3.h>
#include <memory>
#include <Nodes.h>
#include <queue>
#include <SDL3/SDL_vulkan.h>
#include <SDL3_image/SDL_image.h>
#include <span>
#include <UIManager.h>

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
    {"Sponza", GLTF_PATH, GLTF_EXT, "", 1, false, false, false},
    {"DamagedHelmet", GLTF_PATH, GLTF_EXT, "Default_albedo.jpg", 1, false, false, false},
    //{"DamagedHelmet", GLTF_EMBEDDED_PATH, GLTF_EMBEDDED_EXT, "", 1, false, false, false},
    {"SciFiHelmet", GLTF_PATH, GLTF_EXT, "", 1, false, false, false},
};
static std::vector<Vertex> s_GridVertices = {
    {{-0.5f, 0.0f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, 0.0f, -0.5f} , {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{-0.5f, 0.0f, 0.5f} , {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
    {{0.5f, 0.0f, 0.5f}  , {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
};
static std::vector<Uint32> s_GridIndices = {
    0, 1, 2,
    1, 2, 3,
};
static std::vector<glm::vec3> s_PointLightPositions = {
    {0.0f, 2.0f, 0.0f},
};
static unsigned int s_ModelLoadingFlags = aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace;
                                        //| aiProcess_TransformUVCoords | aiProcess_GenBoundingBoxes | aiProcess_CalcTangentSpace;
                                        
static const std::vector<aiTextureType> s_TextureTypes = {
    // These types work for DamagedHelmet
    aiTextureType_BASE_COLOR,
    aiTextureType_NORMALS,
    //aiTextureType_EMISSIVE,
    //aiTextureType_METALNESS,
    //aiTextureType_DIFFUSE_ROUGHNESS,
    //aiTextureType_LIGHTMAP,
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
    SDL_SetGPUSwapchainParameters(mSDLDevice, mWindow, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_MAILBOX);
    ResizeWindow(); // Init color and depth targets and camera aspect ratio

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
    std::filesystem::path basePath = SDL_GetBasePath();
    BasePath = basePath.make_preferred().string() ;
}

bool Renderer::InitPipelines() {
    SDL_GPUShader* vertexShader = LoadShader(mSDLDevice, "PBR.vert", 0, 3, 0, 0);
    if (!vertexShader) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Vertex Shader failed to load");
        return false;
    }

    SDL_GPUShader* fragmentShader = LoadShader(mSDLDevice, "PBR.frag", s_TextureTypes.size(), 1, 0, 0);
    if (!fragmentShader) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Vertex Shader failed to load");
        return false;
    }

    SDL_GPUShader* gridVertShader = LoadShader(mSDLDevice, "Grid.vert", 0, 2, 0, 0);
    if (!gridVertShader) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Grid Vertex Shader failed to load");
        return false;
    }
    SDL_GPUShader* gridFragShader = LoadShader(mSDLDevice, "Grid.frag", 0, 1, 0, 0);
    if (!gridFragShader) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Grid Fragment Shader failed to load");
        return false;
    }

    SDL_GPUColorTargetBlendState colorBlendState{};
    colorBlendState.color_write_mask = SDL_GPU_COLORCOMPONENT_R | SDL_GPU_COLORCOMPONENT_G | SDL_GPU_COLORCOMPONENT_B | SDL_GPU_COLORCOMPONENT_A;
    colorBlendState.enable_blend = false;
    colorBlendState.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    colorBlendState.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendState.color_blend_op = SDL_GPU_BLENDOP_ADD;
    colorBlendState.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    colorBlendState.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
    colorBlendState.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
    SDL_GPUColorTargetDescription colorTargetDescription{};
    colorTargetDescription.format = SDL_GetGPUSwapchainTextureFormat(mSDLDevice, mWindow);
    SDL_GPUColorTargetDescription colorTargetDescription2{};
    colorTargetDescription2.format = colorTargetDescription.format;
    colorTargetDescription2.blend_state = colorBlendState;
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
    vertexBufferDescription.pitch = sizeof(Vertex);
    vertexBufferDescription.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vertexBufferDescription.instance_step_rate = 0;
    std::vector<SDL_GPUVertexBufferDescription> vertexBufferDescriptions{vertexBufferDescription};

    SDL_GPUVertexAttribute vertexPositionAttribute { 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 0};
    SDL_GPUVertexAttribute vertexNormalAttribute   { 1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,     sizeof(glm::vec3)};
    SDL_GPUVertexAttribute vertexTangentAttribute  { 2, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 2 * sizeof(glm::vec3)};
    SDL_GPUVertexAttribute vertexBiTangentAttribute{ 3, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 3 * sizeof(glm::vec3)};
    SDL_GPUVertexAttribute vertexTextureAttribute  { 4, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 4 * sizeof(glm::vec3)};
    std::vector<SDL_GPUVertexAttribute> vertexAttributes{
        vertexPositionAttribute,
        vertexNormalAttribute,
        vertexTangentAttribute,
        vertexBiTangentAttribute,
        vertexTextureAttribute
    };
    SDL_GPUVertexInputState vertexInputState{ vertexBufferDescriptions.data(), 1, vertexAttributes.data(), static_cast<Uint32>(vertexAttributes.size())};

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

    // create grid pipeline
    SDL_GPUGraphicsPipelineCreateInfo gridPipelineCreateInfo{};
    gridPipelineCreateInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    gridPipelineCreateInfo.vertex_shader = gridVertShader;
    gridPipelineCreateInfo.fragment_shader = gridFragShader;
    gridPipelineCreateInfo.target_info = pipelineTargetInfo;
    gridPipelineCreateInfo.vertex_input_state = vertexInputState;
    gridPipelineCreateInfo.depth_stencil_state = depthStencilState;
    gridPipelineCreateInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    gridPipelineCreateInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
    mGridPipeline = SDL_CreateGPUGraphicsPipeline(mSDLDevice, &gridPipelineCreateInfo);
    if (!mGridPipeline) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create 'Grid' graphics pipeline");
        return false;
    }

    SDL_ReleaseGPUShader(mSDLDevice, vertexShader);
    SDL_ReleaseGPUShader(mSDLDevice, fragmentShader);
    SDL_ReleaseGPUShader(mSDLDevice, gridVertShader);
    SDL_ReleaseGPUShader(mSDLDevice, gridFragShader);

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

void Renderer::InitGrid() {
    // copy to mGridMesh
    mGridMesh.vertices = s_GridVertices;
    mGridMesh.indices = s_GridIndices;

    // Create GPU resources
    SDL_GPUBufferCreateInfo vertexBufferCreateInfo{};
    SDL_GPUTransferBuffer* vertexTransferBuffer = nullptr;
    SDL_GPUBufferCreateInfo indexBufferCreateInfo{};
    SDL_GPUTransferBuffer* indexTransferBuffer = nullptr;
    CreateModelGPUResources(mGridMesh, vertexBufferCreateInfo, vertexTransferBuffer, indexBufferCreateInfo, indexTransferBuffer);

    // Upload the transfer data to the vertex buffer
    SDL_GPUCommandBuffer* uploadCmdBuff = SDL_AcquireGPUCommandBuffer(mSDLDevice);
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmdBuff);
    {   // upload vertex data
        SDL_GPUTransferBufferLocation transferBufferLocation{ vertexTransferBuffer, 0 };
        SDL_GPUBufferRegion bufferRegion{ mGridMesh.vertexBuffer, 0, vertexBufferCreateInfo.size };
        SDL_UploadToGPUBuffer(copyPass, &transferBufferLocation, &bufferRegion, false);
    }
    {   // upload index data
        SDL_GPUTransferBufferLocation transferBufferLocation{ indexTransferBuffer, 0 };
        SDL_GPUBufferRegion bufferRegion{ mGridMesh.indexBuffer, 0, indexBufferCreateInfo.size };
        SDL_UploadToGPUBuffer(copyPass, &transferBufferLocation, &bufferRegion, false);
    }

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(uploadCmdBuff);

    SDL_ReleaseGPUTransferBuffer(mSDLDevice, vertexTransferBuffer);
    SDL_ReleaseGPUTransferBuffer(mSDLDevice, indexTransferBuffer);
}

void Renderer::InitMeshes() {
    InitGrid();
    for (auto& model : Models) {
        Mesh mesh;
        if (InitMesh(model, mesh)) {
            mMeshes[model.foldername] = mesh;
        }
        else{
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to initialize mesh of filename: %s", model.foldername.c_str());
        }
    }
    SDL_LogDebug(SDL_LOG_CATEGORY_CUSTOM, "Loaded %zu meshes", mMeshes.size());
}

bool Renderer::InitMesh(const ModelDescriptor& modelDescriptor, Mesh& mesh) {
    MeshLoadingContext context{};
    // Load the model
    if (!LoadModel(modelDescriptor, mesh, context)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load model: %s", SDL_GetError());
        return false;
    }
    // Create Mesh resources
    SDL_GPUBufferCreateInfo vertexBufferCreateInfo{};
    SDL_GPUTransferBuffer* vertexTransferBuffer = nullptr;
    SDL_GPUBufferCreateInfo indexBufferCreateInfo{};
    SDL_GPUTransferBuffer* indexTransferBuffer = nullptr;
    if (!CreateModelGPUResources(mesh, vertexBufferCreateInfo, vertexTransferBuffer, indexBufferCreateInfo, indexTransferBuffer)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create model GPU resources");
        return false;
    }

    const bool bTexturesEmbedded = context.scene->mNumTextures > 0;
    for (size_t i = 0; i < mesh.materials.size(); ++i) {
        MaterialLoadingContext& matInfo = context.materialInfos[i];
        PBRMaterial& meshMat = mesh.materials[i];
        for (auto& [texType, texInfo] : matInfo.textureContextMap) {
            Texture& meshTexture = mesh.textureIdMap[texInfo.filename];
            if (meshTexture.texture) { // We've already loaded it
                aiTextureType otherType = texType;
                aiTextureType typeToAdd = texType;
                if (texType == aiTextureType_DIFFUSE_ROUGHNESS || texType == aiTextureType_METALNESS) {
                    otherType = (texType == aiTextureType_DIFFUSE_ROUGHNESS) ? aiTextureType_METALNESS : aiTextureType_DIFFUSE_ROUGHNESS;
                    typeToAdd = (meshMat.textureMap.contains(texType)) ? otherType : texType;
                }
                else {
                    SDL_assert(meshTexture.type == texType);
                }
                meshMat.textureMap.emplace(typeToAdd, meshTexture);
                matInfo.textureContextMap[typeToAdd] = context.textureInfoMap[matInfo.textureContextMap[otherType].filename];
                continue;
            }
            if (texInfo.bUseFallback) {
                CreateFallbackTexture(texType, meshTexture.texture, texInfo.transferBuffer);
                texInfo.imageSize = {1, 1};
            }
            else {
                if (bTexturesEmbedded) {
                    if (const aiTexture* texture = context.scene->GetEmbeddedTexture(texInfo.filename.c_str())) {
                        const size_t texSize = (texture->mHeight == 0) ? texture->mWidth : texture->mWidth * texture->mHeight;
                        if (SDL_IOStream* ioStream = SDL_IOFromMem(texture->pcData, texSize)) {
                            if (SDL_Surface* image = IMG_LoadTyped_IO(ioStream, true, texture->achFormatHint)) {
                                texInfo.imageData = LoadImageShared(image, 4);
                            }
                        }
                    }
                }
                else {
                    texInfo.imageData = LoadImage(modelDescriptor.foldername, modelDescriptor.subFoldername, texInfo.filename, 4);
                }
                SDL_assert(texInfo.imageData);

                texInfo.imageSize = {static_cast<Uint32>(texInfo.imageData->w), static_cast<Uint32>(texInfo.imageData->h)};
                
                if (!CreateTextureGPUResources(texInfo.imageData, texInfo.filename, meshTexture.texture, texInfo.transferBuffer)) {
                    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create texture GPU resources");
                    return false;
                }
                SDL_assert(meshTexture.texture);
            }
            SDL_assert(texInfo.transferBuffer);
            
            meshTexture.type = texType;
            meshTexture.sampler = mSamplers[1];
            meshMat.textureMap.emplace(texType, meshTexture);
            context.textureInfoMap.emplace(texInfo.filename, texInfo);
        }
        SDL_assert(meshMat.textureMap.size() == s_TextureTypes.size());
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
        for (auto& matInfo : context.materialInfos) {
            for (auto& [texType, texInfo] : matInfo.textureContextMap) {
                SDL_assert(texInfo.transferBuffer);
                SDL_GPUTexture* texture = mesh.textureIdMap[texInfo.filename].texture;
                SDL_GPUTextureTransferInfo textureTransferInfo{ .transfer_buffer = texInfo.transferBuffer, .offset = 0 };
                SDL_GPUTextureRegion textureRegion{ .texture = texture, .w = texInfo.imageSize.x, .h = texInfo.imageSize.y, .d = 1 };
                SDL_UploadToGPUTexture(copyPass, &textureTransferInfo, &textureRegion, false);
            }
        }
    }

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(uploadCmdBuff);

    SDL_ReleaseGPUTransferBuffer(mSDLDevice, vertexTransferBuffer);
    SDL_ReleaseGPUTransferBuffer(mSDLDevice, indexTransferBuffer);
    for (auto& [filename, texInfo] : context.textureInfoMap) {
        SDL_DestroySurface(texInfo.imageData);
        SDL_ReleaseGPUTransferBuffer(mSDLDevice, texInfo.transferBuffer);
    }
    return true;
}

bool Renderer::CreateModelGPUResources(
        Mesh& mesh,
        SDL_GPUBufferCreateInfo& vertexBufferCreateInfo,
        SDL_GPUTransferBuffer*& vertexTransferBuffer,
        SDL_GPUBufferCreateInfo& indexBufferCreateInfo,
        SDL_GPUTransferBuffer*& indexTransferBuffer) {
    
    // Create GPU resources
    vertexBufferCreateInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    vertexBufferCreateInfo.size = static_cast<Uint32>(mesh.vertices.size() * sizeof(Vertex));
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
        std::span transferBufferData{ static_cast<Vertex*>(vertexBufferDataPtr), mesh.vertices.size()};
        std::ranges::copy(mesh.vertices, transferBufferData.begin());

        std::span indexBufferData{ static_cast<Uint32*>(indexBufferDataPtr), mesh.indices.size()};
        std::ranges::copy(mesh.indices, indexBufferData.begin());
    }

    SDL_UnmapGPUTransferBuffer(mSDLDevice, vertexTransferBuffer);
    SDL_UnmapGPUTransferBuffer(mSDLDevice, indexTransferBuffer);
    return true;
}

bool Renderer::CreateTextureGPUResources(
        const SDL_Surface* imageData,
        const std::string textureName,
        SDL_GPUTexture*& outTexture,
        SDL_GPUTransferBuffer*& outTransferBuffer) {

    SDL_GPUTextureCreateInfo createInfo = {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
        .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = static_cast<Uint32>(imageData->w),
        .height = static_cast<Uint32>(imageData->h),
        .layer_count_or_depth = 1,
        .num_levels = 1,
    };
    outTexture = SDL_CreateGPUTexture(mSDLDevice, &createInfo);
    if (!outTexture) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create texture: %s", SDL_GetError());
        return false;
    }
    SDL_SetGPUTextureName(mSDLDevice, outTexture, textureName.c_str());

    // Set the texture data
    SDL_GPUTransferBufferCreateInfo textureTransferBufferCreateInfo{
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = static_cast<Uint32>(imageData->h * imageData->w * 4),
    };
    outTransferBuffer = SDL_CreateGPUTransferBuffer(mSDLDevice, &textureTransferBufferCreateInfo);
    void* textureDataPtr = static_cast<Uint8*>(SDL_MapGPUTransferBuffer(mSDLDevice, outTransferBuffer, false));
    if (!textureDataPtr) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to map GPU transfer buffer data pointer for texture");
        return false;
    }
    else {
        SDL_memcpy(textureDataPtr, imageData->pixels, textureTransferBufferCreateInfo.size);
    }
    SDL_UnmapGPUTransferBuffer(mSDLDevice, outTransferBuffer);
    
    return true;
}

bool Renderer::CreateFallbackTexture(
    aiTextureType type, 
    SDL_GPUTexture*& outTexture, 
    SDL_GPUTransferBuffer*& outTransferBuffer
) {
    SDL_Surface* surface = SDL_CreateSurface(1, 1, SDL_PIXELFORMAT_ABGR8888);
    // TODO: Figure out how to fill the pixel with the appropriate color information
    // this will depend on the type of texture, and we should follow the gltf spec
    // For emissive: https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#_material_emissivetexture

    std::string name = std::format("{}-Fallback", aiTextureTypeToString(type));
    bool success = CreateTextureGPUResources(surface, name, outTexture, outTransferBuffer);

    SDL_DestroySurface(surface);
    return success;
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
    return LoadImage(modelDescriptor.foldername, modelDescriptor.subFoldername, modelDescriptor.textureFilename, desiredChannels);
}

SDL_Surface* Renderer::LoadImage(const std::string& foldername, const std::string& subfoldername, const std::string& texturename, int desiredChannels) {
    // Construct the full path
    std::filesystem::path texturePath = std::format("{}/Content/Models/{}/{}/{}", BasePath, foldername, subfoldername, texturename);
    std::string filePathString = texturePath.make_preferred().string();
    SDL_Surface* image = IMG_Load(filePathString.c_str());
    SDL_assert(image);
    return LoadImageShared(image, desiredChannels);
}

SDL_Surface* Renderer::LoadImageShared(SDL_Surface* image, int desiredChannels) {
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

bool Renderer::LoadModel(const ModelDescriptor& modelDescriptor, Mesh& outMesh, MeshLoadingContext& outContext) {
    // Construct the full path
    std::filesystem::path modelPath = std::format("{}Content/Models/{}/{}/{}{}", BasePath, modelDescriptor.foldername, modelDescriptor.subFoldername, modelDescriptor.foldername, modelDescriptor.fileExtension);
    std::string modelPathString = modelPath.make_preferred().string();
    // Load the model
    
    const aiScene* scene = outContext.importer.ReadFile(modelPathString, s_ModelLoadingFlags);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->HasMeshes()) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load model: %s \nmodel filepath: %s", outContext.importer.GetErrorString(), modelPathString.c_str());
        return false;
    }
    outContext.scene = scene;

    ParseVertices(scene, modelDescriptor.flipX, modelDescriptor.flipY, modelDescriptor.flipZ, outMesh, outContext);
    ParseNodes(outMesh, outContext);
    ParseMaterials(scene, outMesh, outContext);
    ParseTextures(scene, outMesh, outContext);
    //SDL_assert(outMesh.textureIdMap.size() == outContext.textureInfoMap.size());
    outMesh.samplerTypeIndex = modelDescriptor.samplerTypeIndex;
    outMesh.filepath = modelPathString;
    outMesh.bDoNotRender = scene->mNumTextures > 0;
    return true;
}

void Renderer::ParseNodes(Mesh& outMesh, MeshLoadingContext& outContext) {
    int totalChildMeshes = 0;
    int parentId = -1;
    int nodeId = 0;
    NodeLoadingContext rootNode(outContext.scene->mRootNode);
    std::queue<NodeLoadingContext> queue;
    queue.push(rootNode);
    while (!queue.empty()) {
        NodeLoadingContext& curr = queue.front();
        outContext.nodeInfoMap.emplace(curr.pNode->mName.C_Str(), curr);
        queue.pop();

        SceneNode scenenode{parentId, nodeId};

        totalChildMeshes += curr.pNode->mNumMeshes;
        // Set transform for meshes in this node;
        aiMatrix4x4 currMatrix = curr.pNode->mTransformation;
        aiVector3f pos, rot, scale;
        currMatrix.Decompose(scale, rot, pos);
        glm::mat4 nodeTransform = glm::mat4(1.0f);
        nodeTransform = glm::translate(nodeTransform, glm::vec3(pos.x, pos.y, pos.z));
        nodeTransform = glm::rotate(nodeTransform, rot.z, glm::vec3(0, 0, 1));
        nodeTransform = glm::rotate(nodeTransform, rot.y, glm::vec3(0, 1, 0));
        nodeTransform = glm::rotate(nodeTransform, rot.x, glm::vec3(1, 0, 0));
        nodeTransform = glm::scale(nodeTransform, glm::vec3(scale.x, scale.y, scale.z));
        //scenenode.transformation[0][0] = curr.pNode->mTransformation[0][0];
        //scenenode.transformation[0][1] = curr.pNode->mTransformation[0][1];
        //scenenode.transformation[0][2] = curr.pNode->mTransformation[0][2];
        //scenenode.transformation[0][3] = curr.pNode->mTransformation[0][3];
        //scenenode.transformation[1][0] = curr.pNode->mTransformation[1][0];
        //scenenode.transformation[1][1] = curr.pNode->mTransformation[1][1];
        //scenenode.transformation[1][2] = curr.pNode->mTransformation[1][2];
        //scenenode.transformation[1][3] = curr.pNode->mTransformation[1][3];
        //scenenode.transformation[2][0] = curr.pNode->mTransformation[2][0];
        //scenenode.transformation[2][1] = curr.pNode->mTransformation[2][1];
        //scenenode.transformation[2][2] = curr.pNode->mTransformation[2][2];
        //scenenode.transformation[2][3] = curr.pNode->mTransformation[2][3];
        //scenenode.transformation[3][0] = curr.pNode->mTransformation[3][0];
        //scenenode.transformation[3][1] = curr.pNode->mTransformation[3][1];
        //scenenode.transformation[3][2] = curr.pNode->mTransformation[3][2];
        //scenenode.transformation[3][3] = curr.pNode->mTransformation[3][3];
        //scenenode.transformation = glm::transpose(scenenode.transformation);
        scenenode.transformation = nodeTransform;
        
        outMesh.nodeMap.emplace(nodeId, scenenode);
        parentId = nodeId;
        ++nodeId;

        // Queue child nodes
        for (size_t i = 0; i < curr.pNode->mNumChildren; ++i) {
            NodeLoadingContext child(curr.pNode->mChildren[i]);
            queue.push(child);
        }
    }
    //SDL_assert(totalChildMeshes == outContext.scene->mNumMeshes);
    if (totalChildMeshes == outContext.scene->mNumMeshes) {
        UpdateCachedTransformations(outMesh);
    }
    else {
        SDL_LogWarn(SDL_LOG_CATEGORY_CUSTOM, "WARNING: Model has mismatch between number of meshes and meshes in nodes [%i] != [%i]", outContext.scene->mNumMeshes, totalChildMeshes);
        
        for (size_t i = 0; i < outContext.scene->mNumMeshes; ++i) {
            outMesh.submeshes[i].transformation = glm::mat4(1.0f);
        }
    }
}

void Renderer::ParseVertices(const aiScene* scene, const bool flipX, const bool flipY, const bool flipZ, Mesh& outMesh, MeshLoadingContext& outContext) {
    const float xMod = flipX ? -1.0f : 1.0f;
    const float yMod = flipY ? -1.0f : 1.0f;
    const float zMod = flipZ ? -1.0f : 1.0f;

    uint32_t totalVertices = 0;
    uint32_t totalIndices = 0;

    outMesh.submeshes.resize(scene->mNumMeshes);

    for (size_t i = 0; i < outMesh.submeshes.size(); ++i) {
        auto mesh = scene->mMeshes[i];
        outMesh.submeshes[i].materialIndex = mesh->mMaterialIndex;
        outMesh.submeshes[i].numVertices = mesh->mNumVertices;
        outMesh.submeshes[i].numIndices = mesh->mNumFaces * 3;
        outMesh.submeshes[i].baseVertex = totalVertices;
        outMesh.submeshes[i].baseIndex = totalIndices;

        totalVertices += outMesh.submeshes[i].numVertices;
        totalIndices  += outMesh.submeshes[i].numIndices;

        if (mesh->HasPositions()) {
            const aiVector3D zero3D(0.0f, 0.0f, 0.0f);
            outMesh.vertices.reserve(mesh->mNumVertices);
            for (size_t j = 0; j < mesh->mNumVertices; ++j) {
                aiVector3D position = mesh->mVertices[j];
                aiVector3D normal = (mesh->HasNormals()) ? mesh->mNormals[j] : aiVector3D(0.0f, 1.0f, 0.0f);
                aiVector3D tangent = (mesh->HasTangentsAndBitangents()) ? mesh->mTangents[j] : aiVector3D(0.0f);
                aiVector3D bitangent = (mesh->HasTangentsAndBitangents()) ? mesh->mBitangents[j] : aiVector3D(0.0f);
                aiVector3D uv = (mesh->HasTextureCoords(0)) ? mesh->mTextureCoords[0][j] : zero3D;
                outMesh.vertices.push_back({ 
                    .position = {
                        position.x * xMod,
                        position.y * yMod,
                        position.z * zMod
                    },
                    .normal = {
                        normal.x * xMod,
                        normal.y * yMod,
                        normal.z * zMod
                    },
                    .tangent = {
                        tangent.x * xMod,
                        tangent.y * yMod,
                        tangent.z * zMod
                    },
                    .bitangent = {
                        bitangent.x * xMod,
                        bitangent.y * yMod,
                        bitangent.z * zMod
                    },
                    .uv = {
                        uv.x, 
                        uv.y
                    }
                });
                // TODO: Convert from Array-Of-Vertices to Mesh-of-Arrays
                // v_positions.pushback(info);
                // v_normals.pushback(info);
                // v_uv.pushback(info);
            }
            for (size_t j = 0; j < mesh->mNumFaces; ++j) {
                auto face = mesh->mFaces[j];
                SDL_assert(face.mNumIndices == 3);
                outMesh.indices.reserve(face.mNumIndices);
                outMesh.indices.push_back(face.mIndices[0]);
                outMesh.indices.push_back(face.mIndices[1]);
                outMesh.indices.push_back(face.mIndices[2]);
            }
        }
    }
}

// TODO: Is this necessary? How do I detect information rather than hardcode?
void Renderer::ParseMaterials(const aiScene* scene, Mesh& outMesh, MeshLoadingContext& outContext) {
    // const bool bIsBinary = (scene->mNumTextures > 0);
    // for (size_t i = 0; i < scene->mNumMaterials; ++i) {
    //     auto material = scene->mMaterials[i];
    //     if (material) {
    //         aiColor4D baseColor;
    //         material->Get(AI_MATKEY_BASE_COLOR, baseColor);
    //         bool bUseMetallic;
    //         if (material->Get(AI_MATKEY_USE_METALLIC_MAP, bUseMetallic) == aiReturn_SUCCESS) {
    //             material->Get(AI_MATKEY_METALLIC_TEXTURE, bUseMetallic);
    //         }
    //         bool bUseRoughness;
    //         material->Get(AI_MATKEY_USE_ROUGHNESS_MAP, bUseRoughness);
    //         if (bUseRoughness) {
    //             material->Get(AI_MATKEY_ROUGHNESS_FACTOR, )
    //         }
    //     }
    // }
}

void Renderer::ParseTextures(const aiScene* scene, Mesh& outMesh, MeshLoadingContext& outContext) {
    outMesh.materials.resize(scene->mNumMaterials);
    for (size_t i = 0; i < outMesh.materials.size(); ++i) {
        auto material = scene->mMaterials[i];
        if (material) {
            MaterialLoadingContext materialContext{};
            aiString texturePath;
            for (aiTextureType type : s_TextureTypes) {
                TextureLoadingContext textureContext{};
                textureContext.type = type;
                if (material->GetTexture(type, 0, &texturePath) == AI_SUCCESS) {
                    textureContext.filename = texturePath.data;
                }
                else {
                    textureContext.bUseFallback = true;
                    textureContext.filename = aiTextureTypeToString(type);
                }
                materialContext.textureContextMap.emplace(type, textureContext);
            }
            SDL_assert(materialContext.textureContextMap.size() == s_TextureTypes.size());

            outContext.materialInfos.push_back(materialContext);
        }
    }
    SDL_assert(outMesh.materials.size() == outContext.materialInfos.size());
}

void Renderer::Clear() {
}

// This is ugly. I'm passing the UIManager in because 
// I haven't figured out how to do multiple render passes.
void Renderer::Render(UIManager* uiManager) {
    RenderPassContext context{};
    if (!BeginRenderPass(context)) {
        EndRenderPass(context);
        return;
    }
    uiManager->BeginFrame();

    CameraGPU cameraData{};
    InitCameraData(mCameraNodes[0], context.cameraData);

    RecordGridCommands(context);
    RecordModelCommands(context);
    RecordUICommands(context);

    EndRenderPass(context);
}

bool Renderer::BeginRenderPass(RenderPassContext& context) {
    context.commandBuffer = SDL_AcquireGPUCommandBuffer(mSDLDevice);
    if (!context.commandBuffer) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_AcquireGPUCommandBuffer failed: %s", SDL_GetError());
        mNodesThisFrame.clear();
        return false;
    }

    if (!SDL_WaitAndAcquireGPUSwapchainTexture(context.commandBuffer, mWindow, &context.swapchainTexture, nullptr, nullptr)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_WaitAndAcquireGPUSwapchainTexture failed: %s", SDL_GetError());
        mNodesThisFrame.clear();
        return false;
    }
    return true;
}

void Renderer::InitCameraData(const CameraNode* cameraNode, CameraGPU& outCameraData) const {
    auto& camera = cameraNode->mCamera;
    outCameraData.view = camera->mViewMatrix;
    outCameraData.projection = camera->mProjectionMatrix;
    outCameraData.viewProjection = outCameraData.projection * outCameraData.view;
    outCameraData.viewPosition = cameraNode->mTransform->mPosition;
}

void Renderer::RecordGridCommands(RenderPassContext& context) {
    SDL_GPUColorTargetInfo colorTarget{};
    colorTarget.texture = context.swapchainTexture;
    colorTarget.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTarget.store_op = SDL_GPU_STOREOP_STORE;
    colorTarget.layer_or_depth_plane = 0;
    colorTarget.clear_color = SDL_FColor{0.3f,0.2f,0.2f,1.0f};
    std::vector<SDL_GPUColorTargetInfo> colorTargets {colorTarget};

    SDL_GPUDepthStencilTargetInfo depthStencilTarget{};
    depthStencilTarget.texture = mDepthTexture;
    depthStencilTarget.clear_depth = 1.0f;
    depthStencilTarget.load_op = SDL_GPU_LOADOP_CLEAR;
    depthStencilTarget.store_op = SDL_GPU_STOREOP_STORE;
    depthStencilTarget.clear_stencil = 0;
    SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(context.commandBuffer, colorTargets.data(), static_cast<Uint32>(colorTargets.size()), &depthStencilTarget);
    if (!renderPass) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_BeginGPURenderPass failed: %s", SDL_GetError());
        return;
    }
    // Draw Grid
    SDL_BindGPUGraphicsPipeline(renderPass, mGridPipeline);
    std::vector<SDL_GPUBufferBinding> gridBindings{{mGridMesh.vertexBuffer, 0}};
    SDL_BindGPUVertexBuffers(renderPass, 0, gridBindings.data(), static_cast<Uint32>(gridBindings.size()));
    SDL_GPUBufferBinding gridIndexBufferBinding{mGridMesh.indexBuffer, 0};
    SDL_BindGPUIndexBuffer(renderPass, &gridIndexBufferBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
    
    glm::mat4 gridModelMatrix = glm::mat4(1.0f);
    //SDL_PushGPUVertexUniformData(context.commandBuffer, 0, &context.cameraData, sizeof(CameraGPU));
    //SDL_PushGPUVertexUniformData(context.commandBuffer, 1, &gridModelMatrix, sizeof(glm::mat4));
    
    GridParamsFragGPU gridParamsFragGPU{};
    gridParamsFragGPU.offset = glm::vec2(0.0f, 0.0f);
    gridParamsFragGPU.numCells = 16;
    gridParamsFragGPU.thickness = 0.0125f;
    gridParamsFragGPU.scroll = 5.0f;
    //SDL_PushGPUFragmentUniformData(context.commandBuffer, 0, &gridParamsFragGPU, sizeof(GridParamsFragGPU));
    //SDL_DrawGPUIndexedPrimitives(renderPass, static_cast<Uint32>(mGridMesh.indices.size()), 1, 0, 0, 0);

    SDL_EndGPURenderPass(renderPass);
}

void Renderer::RecordModelCommands(RenderPassContext& context) {
    SDL_GPUColorTargetInfo colorTarget{};
    colorTarget.texture = context.swapchainTexture;
    colorTarget.load_op = SDL_GPU_LOADOP_LOAD;
    colorTarget.store_op = SDL_GPU_STOREOP_STORE;
    colorTarget.layer_or_depth_plane = 0;
    colorTarget.clear_color = SDL_FColor{0.3f,0.2f,0.2f,1.0f};
    std::vector<SDL_GPUColorTargetInfo> colorTargets {colorTarget};
    SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;

    SDL_GPUDepthStencilTargetInfo depthStencilTarget{};
    depthStencilTarget.texture = mDepthTexture;
    depthStencilTarget.clear_depth = 1.0f;
    depthStencilTarget.load_op = SDL_GPU_LOADOP_CLEAR;
    depthStencilTarget.store_op = SDL_GPU_STOREOP_STORE;
    depthStencilTarget.clear_stencil = 0;
    
    SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(context.commandBuffer, colorTargets.data(), static_cast<Uint32>(colorTargets.size()), &depthStencilTarget);
    if (!renderPass) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_BeginGPURenderPass failed: %s", SDL_GetError());
        mNodesThisFrame.clear();
        return;
    }
    // Draw Grid here? TBD

    // TEMP
    static SceneLighting s_lighting;
    if (!s_lighting.inited) {
        s_lighting.inited = true;
        s_lighting.pointLights[0].position = {0.0f, 2.0f, 0.0f};
        s_lighting.pointLights[0].enabled = true;
    }
    
    SDL_BindGPUGraphicsPipeline(renderPass, mPipelines[mRenderMode]);
    // Draw Meshes
    for (auto& node : mNodesThisFrame) {
        if (!node->mDisplay->mShow) continue;
        
        const Mesh& mesh = *(node->mDisplay->mMesh);
        const TransformComponent& transform = *(node->mTransform);
        SDL_GPUTexture* diffuseTexture = GetTexture(mesh, aiTextureType_BASE_COLOR);
        std::vector<SDL_GPUBufferBinding> vertexBufferBindings{{mesh.vertexBuffer, 0}};
        SDL_BindGPUVertexBuffers(renderPass, 0, vertexBufferBindings.data(), static_cast<Uint32>(vertexBufferBindings.size()));
        SDL_GPUBufferBinding indexBufferBinding{mesh.indexBuffer, 0};
        SDL_BindGPUIndexBuffer(renderPass, &indexBufferBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

        for (const MeshEntry& submesh : mesh.submeshes) {
            const PBRMaterial& material = mesh.materials[submesh.materialIndex];
            std::vector<SDL_GPUTextureSamplerBinding> samplerBindings;
            GetValidTextureBindings(material, samplerBindings);
            SDL_BindGPUFragmentSamplers(renderPass, 0, samplerBindings.data(), static_cast<Uint32>(samplerBindings.size()));
    
            // model matrix
            glm::mat4 modelMatrix = submesh.transformation;
            modelMatrix = glm::translate(modelMatrix, transform.mPosition);
            modelMatrix = glm::rotate(modelMatrix, glm::radians(transform.mRotation.z), glm::vec3(0, 0, 1));
            modelMatrix = glm::rotate(modelMatrix, glm::radians(transform.mRotation.y), glm::vec3(0, 1, 0));
            modelMatrix = glm::rotate(modelMatrix, glm::radians(transform.mRotation.x), glm::vec3(1, 0, 0));
            modelMatrix = glm::scale(modelMatrix, transform.mScale * glm::vec3(mScale));
            
            SDL_PushGPUVertexUniformData(context.commandBuffer, 0, &context.cameraData, sizeof(CameraGPU));
            SDL_PushGPUVertexUniformData(context.commandBuffer, 1, &modelMatrix, sizeof(glm::mat4));
            SDL_PushGPUVertexUniformData(context.commandBuffer, 2, &s_lighting.pointLights[0].position, sizeof(glm::vec3));
            SDL_PushGPUFragmentUniformData(context.commandBuffer, 0, &s_lighting.pointLights[0].color, sizeof(glm::vec3));
    
            SDL_DrawGPUIndexedPrimitives(renderPass, static_cast<Uint32>(submesh.numIndices), 1, submesh.baseIndex, submesh.baseVertex, 0);
        }
    }

    SDL_EndGPURenderPass(renderPass);
}

void Renderer::RecordUICommands(RenderPassContext& context) {
    ImGui::Render();
    ImDrawData* drawData = ImGui::GetDrawData();
    const bool bUIMinimized = (drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f);

    if (!bUIMinimized) {
        // This is mandatory: call ImGui_ImplSDLGPU3_PrepareDrawData() to upload the vertex/index buffer!
        ImGui_ImplSDLGPU3_PrepareDrawData(drawData, context.commandBuffer);
    }

    SDL_GPUColorTargetInfo colorTarget{};
    colorTarget.texture = context.swapchainTexture;
    colorTarget.load_op = SDL_GPU_LOADOP_LOAD;
    colorTarget.store_op = SDL_GPU_STOREOP_STORE;
    colorTarget.layer_or_depth_plane = 0;
    colorTarget.clear_color = SDL_FColor{0.3f,0.2f,0.2f,1.0f};
    std::vector<SDL_GPUColorTargetInfo> colorTargets {colorTarget};
    SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(context.commandBuffer, colorTargets.data(), static_cast<Uint32>(colorTargets.size()), nullptr);
    if (!renderPass) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_BeginGPURenderPass failed: %s", SDL_GetError());
        return;
    }
    // Draw UI
    if (!bUIMinimized) {
        ImGui_ImplSDLGPU3_RenderDrawData(drawData, context.commandBuffer, renderPass);
    }
    SDL_EndGPURenderPass(renderPass);
}

void Renderer::EndRenderPass(RenderPassContext& context) {
    if (!SDL_SubmitGPUCommandBuffer(context.commandBuffer)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_SubmitGPUCommandBuffer failed: %s", SDL_GetError());
    }
    context.commandBuffer = nullptr;
    context.swapchainTexture = nullptr;

    mNodesThisFrame.clear();
}

void Renderer::Shutdown() {
    mPipelines[RenderMode::count] = mGridPipeline;
    mMeshes["Grid"] = mGridMesh;

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
        for (auto [type, meshTexture] : mesh.textureIdMap) {
            if (meshTexture.texture) SDL_ReleaseGPUTexture(mSDLDevice, meshTexture.texture);
        }
    }
    if (mDepthTexture) SDL_ReleaseGPUTexture(mSDLDevice, mDepthTexture);
    if (mWindow) SDL_DestroyWindow(mWindow);
}

void Renderer::ResizeWindow() {
    if (mDepthTexture) SDL_ReleaseGPUTexture(mSDLDevice, mDepthTexture);
    int windowWidth, windowHeight;
    if (!SDL_GetWindowSize(mWindow, &windowWidth, &windowHeight)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_GetWindowSize failed: %s", SDL_GetError());
        return;
    }
    mCachedWindowCenter = {windowWidth/2, windowHeight/2};
    SDL_GPUTextureCreateInfo colorTextureCreateInfo{
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GetGPUSwapchainTextureFormat(mSDLDevice, mWindow),
        .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
        .width = static_cast<Uint32>(windowWidth),
        .height = static_cast<Uint32>(windowHeight),
        .layer_count_or_depth = 1,
        .num_levels = 1,
        .sample_count = SDL_GPU_SAMPLECOUNT_1,
    };

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

    if (mCameraNodes.size() == 0) return;
    mCameraNodes[0]->mCamera->mAspectRatio = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);
}

void Renderer::SetCameraEntity(CameraNode* cameraNode) {
    if (mCameraNodes.size() > 0)  return;

    mCameraNodes.push_back(cameraNode);
    
    int windowWidth, windowHeight;
    if (!SDL_GetWindowSize(mWindow, &windowWidth, &windowHeight)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_GetWindowSize failed: %s", SDL_GetError());
        return;
    }
    mCameraNodes[0]->mCamera->mAspectRatio = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);
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