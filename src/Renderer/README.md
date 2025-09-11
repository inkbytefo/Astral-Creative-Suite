# Astral Engine Renderer

## Overview

The Astral Engine Renderer is a modular rendering system that supports both 2D and 3D rendering for the Astral Creative Suite. It is built on top of Vulkan 1.4 and provides a unified interface for all rendering operations.

## Directory Structure

- `Renderer.h/cpp` - Main renderer interface (backward compatible)
- `RenderSystem.h/cpp` - Central render system manager
- `2D/` - 2D rendering components
- `3D/` - 3D rendering components
- `Vulkan/` - Low-level Vulkan implementations
- `RendererComponents.h` - Convenience header for all components

## Key Features

### 2D Rendering
- Canvas rendering
- Layer compositing
- Brush and tool rendering
- Grid and UI elements
- View transformations (zoom, pan)

### 3D Rendering
- Scene rendering with ECS integration
- PBR material system
- Shadow mapping
- Model loading and rendering
- Post-processing effects

### Vulkan Backend
- Modern Vulkan 1.4 implementation
- Descriptor set management
- Pipeline caching
- Memory management with VMA
- Swap chain management

## Usage

For detailed usage instructions, see the documentation in the `docs/` directory.

## Building

The renderer is built as part of the main Astral Engine build process. See the main project README for build instructions.