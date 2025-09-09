# WARP.md

This file provides guidance to WARP (warp.dev) when working with code in this repository.

## Project Overview

Astral Engine is a modern 3D game engine developed from scratch using C++17 and Vulkan API. The project follows a modular, data-oriented design with Entity-Component-System (ECS) architecture for scene management.

**Current Status:** Phase 2 - 3D World (Mesh and Memory Management) - 90-95% complete

## Essential Commands

### Build and Development

```powershell
# Quick build using the provided batch script (Windows/MSVC)
.\build_msvc.bat

# Manual CMake build process
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release

# Debug build
cmake --build . --config Debug

# Clean build (remove build directory)
rmdir /s /q build
```

### Running and Testing

```powershell
# Run the engine (from build directory)
.\Release\AstralEditor.exe
# or for Debug
.\Debug\AstralEditor.exe

# Run with validation layers (for Vulkan debugging)
$env:VK_LAYER_PATH = "path_to_vulkan_sdk\Bin"
.\Release\AstralEditor.exe
```

### Shader Development

```powershell
# Compile individual shaders manually
glslc shaders/shader.vert -o shaders/shader.vert.spv
glslc shaders/shader.frag -o shaders/shader.frag.spv

# Shaders are automatically compiled during CMake build
```

### Development Debugging

```powershell
# Check if Vulkan SDK is installed
vkvia
vulkaninfo

# Validate Vulkan layers
$env:VK_LAYER_PATH = "C:\VulkanSDK\[version]\Bin"
```

## Code Architecture

### High-Level Structure

The engine follows a layered architecture with clear separation of concerns:

```
AstralEngine/
├── src/
│   ├── Core/           # Engine core systems
│   ├── Platform/       # OS abstraction layer  
│   ├── Renderer/       # Vulkan-based rendering
│   ├── ECS/           # Entity-Component-System
│   └── main.cpp       # Application entry point
├── shaders/           # GLSL shaders
├── assets/           # Game assets (models, textures)
└── external/         # Third-party dependencies
```

### Core Systems

**Core Module (`src/Core/`):**
- `Logger`: Comprehensive logging system with file output and colored console output
- `AssetManager`: Thread-safe asset loading and caching with async operations
- `ConfigManager`: Configuration file management (JSON-based)
- `ThreadPool`: Multi-threaded task execution for asset loading

**Platform Layer (`src/Platform/`):**
- `Window`: SDL3-based window management and input handling

**Renderer (`src/Renderer/`):**
- `Renderer`: Main rendering orchestration and frame management
- `Vulkan/`: Low-level Vulkan abstraction classes
  - `VulkanDevice`: Vulkan device and queue management
  - `VulkanSwapChain`: Swapchain creation and recreation
  - `VulkanPipeline`: Graphics pipeline management
  - `VulkanBuffer`: Buffer creation using VMA (Vulkan Memory Allocator)
  - `DescriptorSetManager`: Dynamic descriptor set allocation
- `Model`: 3D model representation and vertex data management
- `Camera`: Camera transforms and projection matrices
- `Material`/`MaterialInstance`: Shader parameter management
- `Texture`: Image loading and GPU texture creation
- `Shader`: GLSL shader compilation and management

**ECS Module (`src/ECS/`):**
- `Entity`: Game object with unique ID and transform
- `Scene`: Collection of entities and scene management
- `RenderComponent`: Links entities to renderable models and materials
- `Transform`: 3D transformations (position, rotation, scale) with dirty checking

### Memory Management

The engine uses Vulkan Memory Allocator (VMA) for efficient GPU memory management:
- All Vulkan buffers and images are allocated through VMA
- Automatic memory type selection and suballocation
- Built-in memory usage tracking and debugging

### Asset Pipeline

- **Model Loading**: Using tinyobjloader for .obj file parsing
- **Texture Loading**: stb_image for common image formats
- **Shader Compilation**: Automatic GLSL to SPIR-V compilation via CMake
- **Thread-Safe Caching**: All assets are cached with async loading support

### Rendering Pipeline

1. **Initialization**: Vulkan instance → device → swapchain → pipeline creation
2. **Asset Loading**: Models and textures loaded asynchronously via AssetManager
3. **Scene Setup**: Entities with Transform and RenderComponent
4. **Frame Rendering**: 
   - Update uniform buffers (view/projection matrices)
   - Record command buffers for each entity with push constants (model matrix)
   - Submit and present

## Development Guidelines

### Build Requirements

- **CMake**: 3.16 or higher
- **C++ Compiler**: C++17 support required (MSVC recommended on Windows)
- **Vulkan SDK**: 1.4 or higher with validation layers
- **SDL3**: Included in `external/SDL3/`

### Dependencies Structure

Critical external dependencies are included as Git submodules or directly in `external/`:
- **SDL3**: Window and input management
- **Vulkan Memory Allocator (VMA)**: GPU memory management  
- **tinyobjloader**: .obj model loading
- **stb_image**: Image loading
- **shaderc**: GLSL to SPIR-V compilation
- **GLM**: Mathematics library for 3D transformations

### Error Handling and Debugging

- Use `AE_INFO`, `AE_WARN`, `AE_ERROR`, `AE_DEBUG`, `AE_CRITICAL` logging macros
- Enable Vulkan validation layers for development
- Logger automatically creates timestamped log files
- VMA integration includes memory usage tracking

### Performance Considerations

- Entity transforms use dirty flag system to minimize matrix recalculation
- AssetManager implements LRU caching for loaded resources
- Vulkan descriptor sets are dynamically allocated from pools
- Pipeline cache is saved/loaded to reduce shader compilation overhead

### Threading Model

- Main thread handles rendering and UI
- AssetManager uses ThreadPool for async asset loading
- Thread-safe caching with mutexes for each asset type

## Project Status and Next Steps

**Currently Implemented:**
- Complete Vulkan rendering pipeline
- Window management with SDL3
- Basic 3D model rendering capability
- ECS architecture foundation
- Comprehensive logging system
- Thread-safe asset management framework

**In Progress:**
- .obj model file loading (tinyobjloader integration)
- Texture loading and material system completion

**Next Phases:**
- Complete material and texture system
- Expand ECS with additional components
- Scene serialization and loading

The engine is designed for educational purposes and small to medium-scale 3D applications, prioritizing clear architecture and modern graphics programming practices over AAA-level features.
