# CLAUDE.md - SandCastle Game Engine

**AI Assistant Guide for the SandCastle Codebase**

This document provides comprehensive information about the SandCastle game engine project structure, conventions, and development workflows for AI assistants.

---

## Table of Contents

1. [Project Overview](#project-overview)
2. [Codebase Structure](#codebase-structure)
3. [Architecture and Design Patterns](#architecture-and-design-patterns)
4. [Technology Stack](#technology-stack)
5. [Build System](#build-system)
6. [File Conventions](#file-conventions)
7. [Key Code Locations](#key-code-locations)
8. [Development Workflows](#development-workflows)
9. [Coding Standards](#coding-standards)
10. [Common Tasks](#common-tasks)

---

## Project Overview

**SandCastle** is a modern C++23 game engine built as a learning project for CMake, cross-platform development, and game engine architecture. The engine features:

- **Cross-platform rendering** using SDL3's GPU abstraction layer (supports Vulkan, Metal, DirectX 12)
- **Custom Entity-Component-System (ECS)** architecture
- **Physically Based Rendering (PBR)** pipeline with HLSL shaders
- **Modern asset pipeline** supporting glTF 2.0 models
- **Integrated development tools** including ImGui UI and ImGuizmo

**Current Version:** 0.1.0
**Language Standard:** C++23
**Build System:** CMake 3.23+ with presets for Ninja and Visual Studio 2022

---

## Codebase Structure

### Directory Layout

```
SandCastle/
├── .vscode/                    # VS Code configuration
├── Content/                    # Game assets (textures, models, shaders)
│   ├── Images/                 # Textures (BMP, PNG, HDR, compressed formats)
│   │   ├── bcn/               # Block Compressed Normal (BC1-BC7, DDS)
│   │   └── astc/              # ASTC compressed textures
│   ├── Models/                # 3D models in glTF format
│   │   ├── Sponza/            # Test scene
│   │   ├── DamagedHelmet/     # PBR test asset
│   │   └── SciFiHelmet/       # PBR test asset
│   ├── Shaders/               # HLSL shader sources and compiled outputs
│   │   └── Source/            # .hlsl source files
│   └── environments/          # Environment maps (HDR/KTX)
├── Engine/                    # Engine static library
│   ├── Render/                # Rendering data structures
│   ├── Components.cpp/h       # ECS component implementations
│   ├── Engine.cpp/h           # Main engine orchestrator
│   ├── Entity.cpp/h           # Entity container
│   ├── Input.h                # Input state management
│   ├── Interfaces.h           # Base interfaces
│   ├── Nodes.h                # System node caching
│   ├── Renderer.cpp/h         # SDL_GPU rendering backend
│   ├── Systems.cpp/h          # ECS systems
│   └── UIManager.cpp/h        # ImGui integration
├── Game/                      # Sample game executable
│   ├── main.cpp               # Entry point
│   └── Game.cpp/h             # Game wrapper
├── vendor/                    # External dependencies (git submodules)
│   ├── SDL/                   # SDL3 library
│   ├── SDL_image/             # Image loading
│   ├── glm/                   # Math library
│   ├── assimp/                # Asset import
│   ├── imgui/                 # UI framework
│   ├── ImGuizmo/              # 3D gizmo
│   └── flecs/                 # ECS library (not yet integrated)
├── CMakeLists.txt             # Root CMake configuration
├── CMakePresets.json          # CMake build presets
├── copilot-instructions.md    # Coding standards and best practices
└── README.md                  # Project description
```

### Code Organization

**Total Lines of Code:** ~2,918 (Engine + Game)

**Engine Files:**
- 6 `.cpp` implementation files
- 10 `.h` header files
- Organized by functionality (Components, Systems, Rendering, UI)

**Game Files:**
- 2 `.cpp` files (main.cpp, Game.cpp)
- 1 `.h` file (Game.h)
- Serves as sample application and testing ground

---

## Architecture and Design Patterns

### Entity-Component-System (ECS)

SandCastle implements a **custom ECS architecture** (not using the flecs submodule yet).

#### Entities (`Entity.h`, `Entity.cpp`)
- Container for components with unique IDs
- Template-based component management:
  ```cpp
  Entity entity;
  entity.AddComponent<TransformComponent>(position, rotation, scale);
  auto* transform = entity.GetComponent<TransformComponent>();
  ```
- Support for copying and moving entities

#### Components (`Components.h`, `Components.cpp`)
All components inherit from the `Component` base class and implement `IUIViewable`:

- **TransformComponent** - Position, rotation, scale (glm::vec3)
- **VelocityComponent** - Linear and angular velocity
- **DisplayComponent** - Mesh rendering data (mesh ID, material, textures)
- **CameraComponent** - View/projection matrices, camera modes (FirstPerson, ThirdPerson, Free)
- **UIComponent** - ImGui UI rendering callback

#### Systems (`Systems.h`, `Systems.cpp`)
Systems operate on collections of components via **Nodes** (cached component pointers):

- **MoveSystem** (High Priority) - Updates entity positions based on velocity
- **CameraSystem** (Medium Priority) - Manages camera matrices and input
- **RenderSystem** (Medium Priority) - Submits render nodes to renderer
- **UISystem** (Low Priority) - Handles ImGui UI rendering

#### Node Pattern (`Nodes.h`)
Nodes group related components for efficient system processing:

- **MoveNode** = TransformComponent + VelocityComponent
- **RenderNode** = TransformComponent + DisplayComponent
- **CameraNode** = CameraComponent + TransformComponent
- **UINode** = UIComponent

### Rendering Architecture

#### SDL3 GPU Abstraction
- **Backend-agnostic API** supporting Vulkan, Metal, and DirectX 12
- **Automatic backend selection** based on platform
- **Modern GPU features**: descriptor sets, render passes, command buffers

#### Shader System
- **Source Language:** HLSL with cross-compilation
- **Compilation Targets:** SPIRV (Vulkan), MSL (Metal), DXIL (DirectX 12)
- **Shader Stages:** Vertex, Fragment, Compute
- **Register Spaces:**
  - `space1` - Uniform buffers (Camera, Model, Lights)
  - `space2` - PBR textures (6 texture slots)
  - `space3` - Additional uniforms

#### PBR Rendering Pipeline
- **Lighting Model:** Blinn-Phong with PBR textures
- **Support for up to 9 point lights** in the scene
- **Texture Types:**
  - Base Color (Albedo)
  - Normal Map
  - Emissive
  - Metallic
  - Roughness
  - Ambient Occlusion

#### Mesh and Material System
- **Model Format:** glTF 2.0 (loaded via Assimp)
- **Material Properties:** PBR workflow with texture support
- **Vertex Attributes:** Position, Normal, Tangent, UV coordinates
- **GPU Buffers:** Vertex buffers, index buffers, uniform buffers

### Initialization Flow

```
main()
  └─> Game::Run()
        └─> Engine::Init()
              ├─> SDL_Init(VIDEO | EVENTS | CAMERA | AUDIO)
              ├─> Renderer::Init("SandCastle", 1980x1080)
              │     ├─> Create SDL window and GPU device
              │     ├─> Initialize asset loader (Assimp)
              │     ├─> Create graphics pipelines
              │     ├─> Initialize samplers
              │     └─> Load meshes and textures
              ├─> UIManager::Init(window, device)
              ├─> Create Systems (Move, Camera, Render, UI)
              └─> Create Sample Entities (Camera, Sponza, Helmets)
```

### Game Loop Structure

```
while (Engine::IsRunning()) {
    Engine::Run()           // Poll SDL events, process input
    Game::Update(dt)        // Game logic
      └─> Engine::Update(dt)   // Update all systems by priority
    Engine::Draw()          // Render frame
      └─> Renderer::Render()   // Submit draw calls to GPU
}
```

---

## Technology Stack

### Core Dependencies (Git Submodules)

| Library | Purpose | Location |
|---------|---------|----------|
| **SDL3** | Windowing, input, GPU abstraction | `vendor/SDL/` |
| **SDL_image** | Image loading (PNG, JPG, BMP) | `vendor/SDL_image/` |
| **glm** | Mathematics (vectors, matrices) | `vendor/glm/` |
| **assimp** | 3D model import (glTF, FBX, OBJ) | `vendor/assimp/` |
| **imgui** | Immediate mode GUI framework | `vendor/imgui/` |
| **ImGuizmo** | 3D manipulation gizmo | `vendor/ImGuizmo/` |
| **flecs** | High-performance ECS (not yet used) | `vendor/flecs/` |

### Development Tools

- **Build System:** CMake 3.23+
- **Build Generators:** Ninja (default), Visual Studio 2022
- **Shader Compiler:** `shadercross` (HLSL → SPIRV/MSL/DXIL)
- **Debugger Support:** VS Code launch configurations

### Platform Support

- **Operating Systems:** Linux, Windows, macOS (via SDL3)
- **Graphics APIs:** Vulkan, Metal, DirectX 12 (backend-dependent)
- **Compilers:** GCC, Clang, MSVC (C++23 support required)

---

## Build System

### CMake Configuration

#### Root CMakeLists.txt
```cmake
cmake_minimum_required(VERSION 3.5.0)
project(SandCastle VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(vendor)   # Build external dependencies
add_subdirectory(Engine)   # Build engine static library
add_subdirectory(Game)     # Build game executable
```

#### Build Hierarchy
1. **vendor/CMakeLists.txt** → Builds all external libraries
2. **Engine/CMakeLists.txt** → Builds static library `Engine`, links to `vendor`
3. **Game/CMakeLists.txt** → Builds executable `SandCastle`, links to `Engine`

### CMake Presets

Defined in `CMakePresets.json`:

#### Configure Presets
- **default** - Ninja generator, exports compile commands, C++23
- **vs2022** - Visual Studio 2022 generator (x64)

#### Build Presets
- **defaultPreset** - Standard build
- **clean-build** - Clean before building
- **vs2022-debug** - VS2022 Debug configuration
- **vs2022-release** - VS2022 Release configuration

### Build Commands

```bash
# Configure
cmake --preset default        # Ninja build system
cmake --preset vs2022         # Visual Studio 2022

# Build
cmake --build build           # Build default preset
cmake --build build --clean-first  # Clean build

# Visual Studio
cmake --build build/vs2022 --config Debug
cmake --build build/vs2022 --config Release
```

### Post-Build Steps

The `Game/CMakeLists.txt` defines post-build commands:

1. **Shader Compilation:** Runs `Content/Shaders/Source/compile.sh`
   - Compiles `.hlsl` files to `.spv` (Vulkan), `.msl` (Metal), `.dxil` (DirectX)

2. **Asset Copying:** Copies entire `Content/` directory to build output
   - Ensures runtime access to models, textures, and compiled shaders

### Compiler Definitions

- **GLM:**
  - `GLM_ENABLE_EXPERIMENTAL`
  - `GLM_FORCE_DEPTH_ZERO_TO_ONE` (Vulkan depth range)
  - `GLM_FORCE_RADIANS`

- **ImGui:**
  - `IMGUI_DEFINE_MATH_OPERATORS`

---

## File Conventions

### Header Files (`.h`)

- **Extension:** `.h` (NOT `.hpp`)
- **Header Guards:** Use `#pragma once` (no traditional guards)
- **Naming:** Match the class/module name (e.g., `Engine.h` for `Engine` class)
- **Organization:**
  - Interfaces first (abstract base classes)
  - Forward declarations to minimize includes
  - Public API before private implementation details

### Source Files (`.cpp`)

- **Extension:** `.cpp` (NOT `.cc` or `.cxx`)
- **Location:** Same directory as corresponding header
- **Naming:** Match header file name exactly
- **Organization:** Implementation matches header declaration order

### Shader Files (`.hlsl`)

- **Source Location:** `Content/Shaders/Source/`
- **Naming Convention:**
  - `<ShaderName>.<stage>.hlsl`
  - Examples: `PBR.vert.hlsl`, `PBR.frag.hlsl`, `FillTexture.comp.hlsl`
- **Stages:** `.vert` (vertex), `.frag` (fragment), `.comp` (compute)
- **Compiled Output:** Same name with `.spv`, `.msl`, or `.dxil` extension
- **Register Spaces:**
  ```hlsl
  // space1 - Uniform buffers
  cbuffer CameraUniforms : register(b0, space1) { ... }
  cbuffer ModelUniforms : register(b1, space1) { ... }

  // space2 - PBR textures
  Texture2D baseColorTexture : register(t0, space2);
  Texture2D normalTexture : register(t1, space2);
  ```

### Model Assets (glTF)

- **Primary Format:** glTF 2.0 (`.gltf` or `.glb`)
- **Location:** `Content/Models/<ModelName>/glTF/`
- **Structure:** Each model in its own subdirectory
- **Loading:** Via Assimp library in `Renderer::LoadMesh()`

### Texture Assets

- **Formats:** PNG, BMP, HDR, DDS (BCN), ASTC
- **Location:** `Content/Images/`
- **Compressed Textures:**
  - `Content/Images/bcn/` - Block Compressed (BC1-BC7)
  - `Content/Images/astc/` - ASTC compressed

### Configuration Files

- **CMakePresets.json** - Build configuration and presets
- **imgui.ini** - ImGui window layout (auto-generated, gitignored)
- **copilot-instructions.md** - Coding standards reference
- **.vscode/** - VS Code workspace settings (IntelliSense, debugger, tasks)

---

## Key Code Locations

### Entry Points

| Component | File | Key Function |
|-----------|------|--------------|
| **Application Entry** | `Game/main.cpp` | `main()` |
| **Game Loop** | `Game/Game.cpp` | `Game::Run()` |
| **Engine Initialization** | `Engine/Engine.cpp` | `Engine::Init()` |
| **Renderer Initialization** | `Engine/Renderer.cpp` | `Renderer::Init()` |

### Core Systems

| System | Header | Implementation | Description |
|--------|--------|----------------|-------------|
| **Move System** | `Engine/Systems.h` | `Engine/Systems.cpp` | Updates entity positions |
| **Camera System** | `Engine/Systems.h` | `Engine/Systems.cpp` | Manages camera matrices |
| **Render System** | `Engine/Systems.h` | `Engine/Systems.cpp` | Submits render commands |
| **UI System** | `Engine/Systems.h` | `Engine/Systems.cpp` | Renders ImGui interface |

### Components

| Component | Location | Purpose |
|-----------|----------|---------|
| **TransformComponent** | `Engine/Components.h` | Position, rotation, scale |
| **VelocityComponent** | `Engine/Components.h` | Linear and angular velocity |
| **DisplayComponent** | `Engine/Components.h` | Mesh and material data |
| **CameraComponent** | `Engine/Components.h` | Camera view/projection |
| **UIComponent** | `Engine/Components.h` | ImGui UI rendering |

### Rendering Data Structures

| Structure | Location | Purpose |
|-----------|----------|---------|
| **Vertex** | `Engine/Render/RenderStructs.h` | Vertex attributes |
| **Mesh** | `Engine/Render/RenderStructs.h` | Geometry data |
| **Material** | `Engine/Render/RenderStructs.h` | PBR material properties |
| **Light** | `Engine/Render/RenderStructs.h` | Point light data |
| **CameraUniform** | `Engine/Render/RenderStructs.h` | Camera matrices |

### Important Functions

#### Engine Management
- `Engine::Init()` - Engine/Engine.cpp:25
- `Engine::Run()` - Engine/Engine.cpp:150 (event loop)
- `Engine::Update(float dt)` - Engine/Engine.cpp:200 (system updates)
- `Engine::Draw()` - Engine/Engine.cpp:250 (rendering)

#### Renderer Operations
- `Renderer::Init()` - Engine/Renderer.cpp:40 (GPU initialization)
- `Renderer::LoadMesh()` - Engine/Renderer.cpp:300 (glTF loading)
- `Renderer::Render()` - Engine/Renderer.cpp:500 (frame rendering)

#### Entity/Component Operations
- `Entity::AddComponent<T>()` - Engine/Entity.h (template)
- `Entity::GetComponent<T>()` - Engine/Entity.h (template)
- `Engine::CreateEntity()` - Engine/Engine.cpp:180

---

## Development Workflows

### Initial Setup

1. **Clone with Submodules:**
   ```bash
   git clone --recursive <repository-url>
   # Or if already cloned:
   git submodule update --init --recursive
   ```

2. **Configure Build:**
   ```bash
   cmake --preset default
   # Or for Visual Studio:
   cmake --preset vs2022
   ```

3. **Build:**
   ```bash
   cmake --build build
   ```

4. **Run:**
   ```bash
   ./build/SandCastle
   # Or on Windows:
   .\build\SandCastle.exe
   ```

### Development Cycle

1. **Make Code Changes** in `Engine/` or `Game/`
2. **Rebuild:** `cmake --build build`
3. **Test:** Run the executable
4. **Debug:** Use VS Code debugger (F5) with configurations in `.vscode/launch.json`

### Adding a New Component

1. **Declare in `Engine/Components.h`:**
   ```cpp
   class MyComponent : public Component, public IUIViewable {
   public:
       MyComponent();
       void RenderUI() override;
       // Component data...
   };
   ```

2. **Implement in `Engine/Components.cpp`:**
   ```cpp
   MyComponent::MyComponent() { /* initialize */ }
   void MyComponent::RenderUI() { /* ImGui code */ }
   ```

3. **Create Node in `Engine/Nodes.h` (if needed):**
   ```cpp
   struct MyNode : public Node {
       TransformComponent* transform;
       MyComponent* myComponent;
   };
   ```

4. **Create System in `Engine/Systems.h`:**
   ```cpp
   class MySystem : public System<MyNode> {
   public:
       void Update(float deltaTime) override;
   };
   ```

5. **Register in `Engine::Init()`:**
   ```cpp
   AddSystem<MySystem>(SystemPriority::Medium);
   ```

### Adding a New Shader

1. **Create HLSL Source:** `Content/Shaders/Source/MyShader.vert.hlsl`
   ```hlsl
   struct VSInput {
       float3 position : POSITION;
   };

   struct VSOutput {
       float4 position : SV_POSITION;
   };

   VSOutput main(VSInput input) {
       VSOutput output;
       output.position = float4(input.position, 1.0);
       return output;
   }
   ```

2. **Update `compile.sh`:** Add compilation commands for new shader

3. **Rebuild:** Post-build script will compile shader to all backends

4. **Load in Renderer:** Update `Renderer::Init()` to load shader bytecode

### Working with Models

1. **Add Model to `Content/Models/`:**
   - Create subdirectory: `Content/Models/MyModel/glTF/`
   - Place `.gltf` or `.glb` file and textures

2. **Load in Code:**
   ```cpp
   std::string meshID = renderer->LoadMesh("Content/Models/MyModel/glTF/scene.gltf");
   ```

3. **Create Entity with Mesh:**
   ```cpp
   Entity entity = engine->CreateEntity();
   entity.AddComponent<TransformComponent>(position, rotation, scale);
   entity.AddComponent<DisplayComponent>(meshID, material);
   ```

### Debugging

#### VS Code Debug Configuration
- **File:** `.vscode/launch.json`
- **Target:** `SandCastle` executable
- **Working Directory:** `${workspaceFolder}/build`
- **Debugger:** gdb (Linux), lldb (macOS), msvc (Windows)

#### Common Debug Tasks
- **Step through game loop:** Set breakpoint in `Game::Run()`
- **Inspect entities:** Breakpoint in `Engine::Update()`
- **Debug rendering:** Breakpoint in `Renderer::Render()`
- **Check component data:** Use ImGui's UI rendering in `Component::RenderUI()`

### Testing Changes

1. **Visual Testing:** Run the application and observe rendering
2. **ImGui Inspection:** Use built-in UI to inspect component values
3. **Console Logging:** Add `SDL_Log()` statements for debugging
4. **Validation Layers:** Enable GPU validation (Vulkan/D3D12 debug layers)

---

## Coding Standards

Based on `copilot-instructions.md`, follow these best practices:

### General Guidelines

- **Code Readability:** Use meaningful names for variables, functions, and classes
- **Modular Design:** Keep engine modules logically separated
- **Error Handling:** Implement robust error checking and logging
- **Cross-Platform:** Use SDL3 and standard libraries for portability

### C++ Standards

- **Modern C++:** Use C++23 features (requires compiler support)
- **Smart Pointers:** Prefer `std::unique_ptr` and `std::shared_ptr` over raw pointers
- **Const Correctness:** Use `const` wherever applicable
- **Avoid Global State:** Minimize global variables; prefer dependency injection
- **RAII:** Resource Acquisition Is Initialization for automatic cleanup

### Code Style

```cpp
// Class names: PascalCase
class TransformComponent { };

// Function names: PascalCase
void UpdateTransform(float deltaTime);

// Variable names: camelCase
float deltaTime = 0.0f;
int entityCount = 0;

// Member variables: m prefix + PascalCase
class Engine {
    Renderer mRenderer;
    std::vector<Entity> mEntities;
};

// Constants: UPPER_SNAKE_CASE or kPascalCase
constexpr int MAX_LIGHTS = 9;
const float kGravity = 9.81f;

// Namespaces: lowercase
namespace sandcastle { }
```

### Memory Management

- **Use Smart Pointers:**
  ```cpp
  std::unique_ptr<System> system = std::make_unique<MoveSystem>();
  std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>();
  ```

- **Avoid Manual Memory Management:**
  ```cpp
  // BAD
  Mesh* mesh = new Mesh();
  delete mesh;

  // GOOD
  auto mesh = std::make_unique<Mesh>();
  // Automatically cleaned up
  ```

### Rendering Best Practices

- **Batch Draw Calls:** Minimize state changes
- **Shader Management:** Cache compiled shaders
- **Texture Binding:** Use descriptor sets efficiently
- **Buffer Updates:** Minimize GPU-CPU synchronization

### Asset Management

- **Asynchronous Loading:** Load assets without blocking (future work)
- **Resource Caching:** Don't reload the same asset twice
- **Memory Pools:** Consider pooled allocators for frequent allocations

### Component Design

- **Keep Components Data-Oriented:** Minimize logic in components
- **Systems Process Logic:** Move behavior to systems
- **Composition Over Inheritance:** Prefer adding components over deep hierarchies

---

## Common Tasks

### Adding a New Entity Type

```cpp
// In Engine::Init() or Game code:
Entity helmet = CreateEntity();
helmet.AddComponent<TransformComponent>(
    glm::vec3(0.0f, 1.0f, 0.0f),  // position
    glm::vec3(0.0f),               // rotation
    glm::vec3(1.0f)                // scale
);
helmet.AddComponent<DisplayComponent>(
    mRenderer.LoadMesh("Content/Models/DamagedHelmet/glTF/DamagedHelmet.gltf")
);
```

### Changing Camera Mode

```cpp
// Camera modes: FirstPerson, ThirdPerson, Free
CameraComponent* camera = entity.GetComponent<CameraComponent>();
camera->cameraMode = CameraMode::Free;
```

### Adding a Point Light

```cpp
// In Renderer::Render() or material setup:
Light light;
light.position = glm::vec3(5.0f, 3.0f, 0.0f);
light.color = glm::vec3(1.0f, 0.8f, 0.6f);  // Warm white
light.intensity = 10.0f;
light.radius = 15.0f;
// Add to lights array in uniform buffer
```

### Loading a New Texture

```cpp
// In DisplayComponent or Material:
SDL_Surface* surface = IMG_Load("Content/Images/my_texture.png");
SDL_GPUTexture* texture = SDL_CreateGPUTexture(device, &textureDesc);
// Upload texture data to GPU
```

### Modifying the Game Loop

```cpp
// In Game::Update():
void Game::Update(float deltaTime) {
    // Custom game logic here

    // Update engine systems
    mEngine->Update(deltaTime);

    // Additional post-update logic
}
```

### Adding ImGui UI Elements

```cpp
// In a Component's RenderUI() method:
void MyComponent::RenderUI() {
    ImGui::Text("My Component");
    ImGui::DragFloat3("Position", &position.x);
    ImGui::ColorEdit3("Color", &color.r);
    if (ImGui::Button("Reset")) {
        position = glm::vec3(0.0f);
    }
}
```

### Handling Input

```cpp
// In Engine::Run() event loop:
if (event.type == SDL_EVENT_KEY_DOWN) {
    if (event.key.key == SDLK_SPACE) {
        // Handle space key press
    }
}

// Or check current state:
if (mInputState.keys[SDL_SCANCODE_W]) {
    // W key is currently held down
}
```

### Profiling Performance

```cpp
// Add timing code:
auto startTime = std::chrono::high_resolution_clock::now();

// ... code to profile ...

auto endTime = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
SDL_Log("Operation took %lld microseconds", duration.count());
```

---

## Important Notes for AI Assistants

### When Working with This Codebase:

1. **Always Preserve C++23:** Don't downgrade to older C++ standards
2. **Respect the ECS Pattern:** Keep components data-focused, logic in systems
3. **Test Rendering Changes:** Graphics bugs may not be obvious from code alone
4. **Check Shader Compilation:** HLSL changes require rebuilding with `compile.sh`
5. **Mind the Build System:** Changes to CMakeLists.txt affect the entire build
6. **Coordinate Spaces:** GLM uses OpenGL conventions (right-handed, Y-up)
7. **Depth Range:** Vulkan uses [0, 1] due to `GLM_FORCE_DEPTH_ZERO_TO_ONE`
8. **Submodule Changes:** Avoid modifying vendor libraries directly

### Common Pitfalls:

- **Forgetting to register systems** in `Engine::Init()`
- **Not rebuilding after shader changes** (shaders compile at build time)
- **Incorrect register space** in HLSL shaders
- **Missing ImGui rendering** in component `RenderUI()` methods
- **Not copying new assets** to build directory (handled by CMake post-build)

### Best Practices:

- **Read existing code first** before making changes to understand patterns
- **Follow the established naming conventions** (see Coding Standards)
- **Test incrementally** rather than making large changes at once
- **Use ImGui for debugging** - add temporary UI to inspect values
- **Check console output** - SDL_Log statements are useful for debugging

---

## Additional Resources

- **SDL3 Documentation:** https://wiki.libsdl.org/SDL3/
- **ImGui Documentation:** https://github.com/ocornut/imgui
- **glTF Specification:** https://www.khronos.org/gltf/
- **GLM Documentation:** https://glm.g-truc.net/
- **Assimp Documentation:** https://assimp.sourceforge.net/

---

## Changelog

**Version 1.0** (2025-11-14)
- Initial CLAUDE.md creation
- Comprehensive documentation of current codebase state
- Documented ECS architecture, rendering pipeline, build system
- Added common workflows and coding standards

---

**Last Updated:** 2025-11-14
**Engine Version:** 0.1.0
**Maintainer:** SandCastle Development Team
