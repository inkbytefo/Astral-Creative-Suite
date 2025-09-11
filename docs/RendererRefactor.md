r # Astral Engine Renderer Refactoring Documentation

## Overview

This document describes the refactored renderer architecture for the Astral Creative Suite, which now supports both 2D and 3D rendering in a unified, modular system.

## Architecture

### New Directory Structure

```
src/Renderer/
├── Renderer.h/cpp              # Main renderer interface (backward compatibility)
├── RenderSystem.h/cpp          # Render system manager
├── RendererComponents.h        # Convenience header for all renderer components
├── 2D/
│   ├── CanvasRenderer.h/cpp    # 2D canvas rendering
│   ├── LayerRenderer.h/cpp     # 2D layer rendering
│   ├── BrushRenderer.h/cpp     # 2D brush rendering
│   └── GridRenderer.h/cpp      # 2D grid rendering
├── 3D/
│   ├── SceneRenderer.h/cpp     # 3D scene rendering
│   ├── ModelRenderer.h/cpp     # 3D model rendering
│   ├── MaterialRenderer.h/cpp  # Material system
│   └── ShadowRenderer.h/cpp    # Shadow mapping
├── Vulkan/                     # Vulkan backend (unchanged)
│   ├── VulkanContext.h/cpp
│   ├── VulkanDevice.h/cpp
│   ├── VulkanSwapChain.h/cpp
│   ├── VulkanPipeline.h/cpp
│   ├── VulkanBuffer.h/cpp
│   ├── VulkanUtils.h/cpp
│   └── DescriptorSetManager.h/cpp
├── Shaders/                    # Shader management
│   ├── ShaderManager.h/cpp
│   ├── ShaderVariant.h/cpp
│   └── ShaderCompiler.h/cpp
├── Materials/                  # Material system
│   ├── Material.h/cpp
│   ├── Texture.h/cpp
│   └── UniformBuffer.h/cpp
└── CMakeLists.txt              # Build configuration
```

### Key Components

#### 1. RenderSystem
The central coordinator for all rendering operations, managing both 2D and 3D rendering contexts.

#### 2. CanvasRenderer (2D)
Handles 2D canvas rendering operations including:
- Canvas background rendering
- Layer compositing
- Brush strokes
- Grid rendering
- View transformations

#### 3. SceneRenderer (3D)
Manages 3D scene rendering with:
- Model rendering
- Material system integration
- Shadow mapping
- Post-processing effects

#### 4. Backward Compatibility
The existing [Renderer.h](file:///c%3A/Users/tpoyr/OneDrive/Desktop/AstralEngine_OLD/src/Renderer/Renderer.h) and [Renderer.cpp](file:///c%3A/Users/tpoyr/OneDrive/Desktop/AstralEngine_OLD/src/Renderer/Renderer.cpp) files have been updated to delegate to the new RenderSystem while maintaining API compatibility.

## Benefits of the Refactor

1. **Modularity**: Clear separation between 2D and 3D rendering systems
2. **Extensibility**: Easy to add new rendering features
3. **Maintainability**: Each component has a single responsibility
4. **Performance**: Optimized rendering paths for each use case
5. **Compatibility**: Existing code continues to work without changes

## Usage

### For 2D Rendering (Creative Suite)
```cpp
// Initialize render system
RenderSystem renderSystem(window);
renderSystem.initialize();

// Get canvas renderer
auto& canvasRenderer = renderSystem.getCanvasRenderer();

// Render a canvas
CanvasRenderData canvasData;
canvasData.width = 1920;
canvasData.height = 1080;
canvasData.backgroundColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
// ... set other canvas properties

std::vector<ECS::EntityID> layers;
canvasRenderer.renderCanvas(canvasData, layers);
```

### For 3D Rendering (Blender-like features)
```cpp
// Get scene renderer
auto& sceneRenderer = renderSystem.getSceneRenderer();

// Render a 3D scene
sceneRenderer.renderScene(scene);
```

### Backward Compatible Usage
```cpp
// Existing code continues to work
Renderer renderer(window);
renderer.drawFrame(scene);
```

## Build System

The refactored renderer is built as a separate library `AstralRenderer` which is linked to the main `AstralEngine` library. This modular approach allows for easier testing and maintenance.

## Future Enhancements

1. **Advanced 2D Features**: Add support for blend modes, filters, and effects
2. **3D Editor**: Implement Blender-like 3D viewport rendering
3. **Render Graph**: Implement a render graph system for complex rendering pipelines
4. **Multi-threading**: Add support for multi-threaded rendering
5. **VR/AR Support**: Extend the renderer for virtual and augmented reality applications

## Testing

A test executable `test_renderer` is included to verify the renderer components can be compiled and linked correctly.