#include "Renderer.h"

#ifdef VULKAN_IMPLEMENTATION
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;
#include <Volk/volk.h>
#endif

#include <fstream>
#include <SDL3/SDL_vulkan.h>
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

    SDL_ShowWindow(mWindow);

    return true;
}

void Renderer::InitAssetLoader() {
    BasePath = SDL_GetBasePath();
}

bool Renderer::InitPipelines() {
    mMesh = {
        {   // vertices
            { {-0.5f,  0.5f, 0}, { 0,  0}}, // NW
            { { 0.5f,  0.5f, 0}, { 4,  0}}, // NE
            { { 0.5f, -0.5f, 0}, { 4,  4}}, // SE
            { {-0.5f, -0.5f, 0}, { 0,  4}}, // SW
        },
        // indices
        {0, 1, 2, 0, 2, 3}
    };
    
    SDL_GPUShader* vertexShader = LoadShader(mSDLDevice, "TexturedQuad.vert", 0, 0, 0, 0);
    if (!vertexShader) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Vertex Shader failed to load");
        return false;
    }

    SDL_GPUShader* fragmentShader = LoadShader(mSDLDevice, "TexturedQuad.frag", 1, 0, 0, 0);
    if (!fragmentShader) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Vertex Shader failed to load");
        return false;
    }

    // Load the image
    SDL_Surface* imageData = LoadImage("ravioli.bmp", 4);
    if (!imageData) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load image: %s", SDL_GetError());
        return false;
    }

    SDL_GPUColorTargetDescription colorTargetDescription{};
    colorTargetDescription.format = SDL_GetGPUSwapchainTextureFormat(mSDLDevice, mWindow);
    std::vector colorTargetDescriptions{colorTargetDescription};
    SDL_GPUGraphicsPipelineTargetInfo pipelineTargetInfo{};
    pipelineTargetInfo.color_target_descriptions = colorTargetDescriptions.data();
    pipelineTargetInfo.num_color_targets = static_cast<Uint32>(colorTargetDescriptions.size());

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

    // Create GPU resources
    SDL_GPUBufferCreateInfo vertexBufferCreateInfo{};
    vertexBufferCreateInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    vertexBufferCreateInfo.size = static_cast<Uint32>(mMesh.vertices.size() * sizeof(PositionTextureVertex));
    mVertexBuffer = SDL_CreateGPUBuffer(mSDLDevice, &vertexBufferCreateInfo);
    if (!mVertexBuffer) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create 'Vertex' buffer");
        return false;
    }
    SDL_SetGPUBufferName(mSDLDevice, mVertexBuffer, "Vertex Buffer");

    SDL_GPUBufferCreateInfo indexBufferCreateInfo{};
    indexBufferCreateInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    indexBufferCreateInfo.size = static_cast<Uint32>(mMesh.indices.size() * sizeof(Uint32));
    mIndexBuffer =  SDL_CreateGPUBuffer(mSDLDevice, &indexBufferCreateInfo);
    if (!mIndexBuffer) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create 'Index' buffer");
        return false;
    }
    SDL_SetGPUBufferName(mSDLDevice, mIndexBuffer, "Index Buffer");

    SDL_GPUTextureCreateInfo textureCreateInfo{
		.type = SDL_GPU_TEXTURETYPE_2D,
		.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
		.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
		.width = static_cast<Uint32>(imageData->w),
		.height = static_cast<Uint32>(imageData->h),
		.layer_count_or_depth = 1,
		.num_levels = 1,
    };
    mTexture = SDL_CreateGPUTexture(mSDLDevice, &textureCreateInfo);
    if (!mTexture) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create texture: %s", SDL_GetError());
        return false;
    }
    SDL_SetGPUTextureName(mSDLDevice, mTexture, "Ravioli Texture");

    SDL_GPUTransferBufferCreateInfo vertexTransferBufferCreateInfo{};
    vertexTransferBufferCreateInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    vertexTransferBufferCreateInfo.size = vertexBufferCreateInfo.size;
    
    SDL_GPUTransferBufferCreateInfo indexTransferBufferCreateInfo{};
    indexTransferBufferCreateInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    indexTransferBufferCreateInfo.size = indexBufferCreateInfo.size;

    SDL_GPUTransferBuffer* vertexTransferBuffer = SDL_CreateGPUTransferBuffer(mSDLDevice, &vertexTransferBufferCreateInfo);
    if (!vertexTransferBuffer) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create GPU vertex transfer buffer");
        return false;
    }
    
    SDL_GPUTransferBuffer* indexTransferBuffer = SDL_CreateGPUTransferBuffer(mSDLDevice, &indexTransferBufferCreateInfo);
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
        std::span transferBufferData{ static_cast<PositionTextureVertex*>(vertexBufferDataPtr), mMesh.vertices.size()};
        std::ranges::copy(mMesh.vertices, transferBufferData.begin());

        std::span indexBufferData{ static_cast<Uint32*>(indexBufferDataPtr), mMesh.indices.size()};
        std::ranges::copy(mMesh.indices, indexBufferData.begin());
    }

    SDL_UnmapGPUTransferBuffer(mSDLDevice, vertexTransferBuffer);
    SDL_UnmapGPUTransferBuffer(mSDLDevice, indexTransferBuffer);

    // Set the texture data
    SDL_GPUTransferBufferCreateInfo textureTransferBufferCreateInfo{
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = static_cast<Uint32>(imageData->h * imageData->w * 4),
    };
    SDL_GPUTransferBuffer* textureTransferBuffer = SDL_CreateGPUTransferBuffer(mSDLDevice, &textureTransferBufferCreateInfo);
    void* textureDataPtr = static_cast<Uint8*>(SDL_MapGPUTransferBuffer(mSDLDevice, textureTransferBuffer, false));
    if (!textureDataPtr) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to map GPU transfer buffer data pointer for texture");
        return false;
    }
    else {
        SDL_memcpy(textureDataPtr, imageData->pixels, textureTransferBufferCreateInfo.size);
    }
    SDL_UnmapGPUTransferBuffer(mSDLDevice, textureTransferBuffer);

    // Upload the transfer data to the vertex buffer
    SDL_GPUCommandBuffer* uploadCmdBuff = SDL_AcquireGPUCommandBuffer(mSDLDevice);
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmdBuff);
    {   // upload vertex data
        SDL_GPUTransferBufferLocation transferBufferLocation{ vertexTransferBuffer, 0 };
        SDL_GPUBufferRegion bufferRegion{ mVertexBuffer, 0, vertexBufferCreateInfo.size };
        SDL_UploadToGPUBuffer(copyPass, &transferBufferLocation, &bufferRegion, false);
    }
    {   // upload index data
        SDL_GPUTransferBufferLocation transferBufferLocation{ indexTransferBuffer, 0 };
        SDL_GPUBufferRegion bufferRegion{ mIndexBuffer, 0, indexBufferCreateInfo.size };
        SDL_UploadToGPUBuffer(copyPass, &transferBufferLocation, &bufferRegion, false);
    }
    {   // upload texture data
        SDL_GPUTextureTransferInfo textureTransferInfo{ .transfer_buffer = textureTransferBuffer, .offset = 0 };
        SDL_GPUTextureRegion textureRegion{ .texture = mTexture, .w = static_cast<Uint32>(imageData->w), .h = static_cast<Uint32>(imageData->h), .d = 1 };
        SDL_UploadToGPUTexture(copyPass, &textureTransferInfo, &textureRegion, false);
    }

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(uploadCmdBuff);
    SDL_DestroySurface(imageData);
    SDL_ReleaseGPUTransferBuffer(mSDLDevice, vertexTransferBuffer);
    SDL_ReleaseGPUTransferBuffer(mSDLDevice, indexTransferBuffer);

    return true;
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

SDL_Surface* Renderer::LoadImage(const std::string& filename, int desiredChannels) {
    std::string fullPath = std::format("{}/Content/Images/{}", BasePath, filename);
    SDL_Surface* image = SDL_LoadBMP(fullPath.c_str());
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
        
        SDL_GPURenderPass* renderPass{
            SDL_BeginGPURenderPass(commandBuffer, colorTargets.data(), static_cast<Uint32>(colorTargets.size()), nullptr)
        };

        SDL_BindGPUGraphicsPipeline(renderPass, mPipelines[mRenderMode]);
        std::vector<SDL_GPUBufferBinding> bindings{{mVertexBuffer, 0}};
        SDL_BindGPUVertexBuffers(renderPass, 0, bindings.data(), static_cast<Uint32>(bindings.size()));
        SDL_GPUBufferBinding indexBufferBinding{mIndexBuffer, 0};
        SDL_BindGPUIndexBuffer(renderPass, &indexBufferBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
        SDL_GPUTextureSamplerBinding textureSamplerBinding{mTexture, mSamplers[mCurrentSamplerIndex]};
        SDL_BindGPUFragmentSamplers(renderPass, 0, &textureSamplerBinding, 1);
        SDL_DrawGPUIndexedPrimitives(renderPass, static_cast<Uint32>(mMesh.indices.size()), 1, 0, 0, 0);
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
    if (mVertexBuffer) SDL_ReleaseGPUBuffer(mSDLDevice, mVertexBuffer);
    if (mIndexBuffer) SDL_ReleaseGPUBuffer(mSDLDevice, mIndexBuffer);
    if (mTexture) SDL_ReleaseGPUTexture(mSDLDevice, mTexture);
    if (mWindow) SDL_DestroyWindow(mWindow);
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
