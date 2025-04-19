# Copilot Instructions: Best Practices for a C++ Game Engine

## General Guidelines
- **Code Readability**: Write clean, readable, and well-documented code. Use meaningful variable and function names.
- **Modular Design**: Break down the engine into logical modules (e.g., Renderer, Physics, Input, etc.) to ensure maintainability and scalability.
- **Error Handling**: Implement robust error handling and logging mechanisms to identify and debug issues efficiently.
- **Cross-Platform Support**: Use libraries like SDL, Vulkan, or OpenGL to ensure compatibility across platforms.

## Coding Standards
- **Follow C++ Standards**: Adhere to modern C++ standards (C++17 or later) for better performance and safety.
- **Use Smart Pointers**: Prefer `std::unique_ptr` and `std::shared_ptr` over raw pointers to manage memory safely.
- **Avoid Global State**: Minimize the use of global variables to reduce coupling and improve testability.
- **Const Correctness**: Use `const` wherever applicable to prevent unintended modifications.

## Rendering
- **Abstraction**: Abstract rendering APIs (e.g., Vulkan, OpenGL) to allow flexibility in switching or supporting multiple backends.
- **Batch Rendering**: Optimize rendering by batching draw calls to reduce overhead.
- **Shader Management**: Use a shader management system to load, compile, and cache shaders efficiently.

## Asset Management
- **Resource Loading**: Implement an asset loader to handle textures, models, and other resources asynchronously.
- **Memory Management**: Use memory pools or allocators for efficient resource management.
- **Hot Reloading**: Support hot reloading of assets to speed up development iterations.

## Physics and Gameplay
- **Component-Based Architecture**: Use an Entity-Component-System (ECS) for flexibility in defining game objects and behaviors.
- **Collision Detection**: Optimize collision detection algorithms for performance.
- **Frame Independence**: Ensure gameplay logic is frame-rate independent by using delta time.

## Debugging and Testing
- **Debug Tools**: Include in-engine debugging tools like frame capture, performance metrics, and logging.
- **Unit Tests**: Write unit tests for critical components to ensure reliability.
- **Profiling**: Use profiling tools to identify and optimize performance bottlenecks.

## Build and Deployment
- **CMake**: Use CMake for cross-platform build configuration.
- **Continuous Integration**: Set up CI pipelines to automate builds and tests.
- **Release Builds**: Optimize release builds with compiler flags and strip debug symbols.

## Documentation
- **Code Comments**: Document classes, functions, and complex logic with comments.
- **User Guides**: Provide documentation for developers and end-users to understand and use the engine.
- **API Reference**: Generate API documentation using tools like Doxygen.

## Community and Collaboration
- **Version Control**: Use Git for version control and maintain a clean commit history.
- **Code Reviews**: Conduct code reviews to ensure quality and share knowledge.
- **Open Source**: If applicable, follow open-source best practices like maintaining a `CONTRIBUTING.md` file.