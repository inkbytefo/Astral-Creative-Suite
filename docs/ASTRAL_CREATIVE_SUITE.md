# Astral Creative Suite Documentation

## Overview

Astral Creative Suite is a comprehensive creative application that combines 2D graphics editing and 3D modeling capabilities in a single environment. Built with C++17 and Vulkan API, it provides a powerful foundation for digital artists and designers.

## Architecture

The Astral Creative Suite follows a modular architecture with the following key components:

### Core Engine
- **Logging System**: Comprehensive logging for debugging and monitoring
- **Memory Management**: Efficient memory allocation and management
- **Asset Management**: Loading and management of various asset types
- **Event System**: Event handling and dispatching
- **Configuration**: Runtime configuration and settings

### Rendering System
- **Vulkan Backend**: High-performance graphics rendering using Vulkan API
- **Shader System**: Flexible shader management with variant support
- **Material System**: Physically-based rendering materials
- **Texture Management**: Loading, processing, and management of textures

### UI System
- **Window Management**: Cross-platform window creation and management
- **UI Framework**: Custom UI components and widgets
- **Input Handling**: Mouse, keyboard, and other input device handling

### 2D Graphics Editor
- **Canvas System**: 2D workspace with zoom, pan, and grid functionality
- **Layer System**: Layer-based editing with blending modes
- **Tool System**: Extensible tool framework with selection, brush, and eraser tools
- **Brush System**: Advanced brush engine with pressure sensitivity

### 3D Modeling Tools
- **Mesh Editing**: Basic mesh manipulation capabilities
- **Viewport System**: 3D viewport navigation and rendering
- **Material Editor**: 3D material creation and editing

## 2D Graphics Editor

### Canvas System
The canvas system provides the foundation for 2D editing:

- **View Manipulation**: Zoom, pan, and reset view functionality
- **Grid System**: Configurable grid display for precise alignment
- **Coordinate System**: Screen-to-world and world-to-screen coordinate conversion

### Layer System
The layer system implements industry-standard layer functionality:

- **Layer Operations**: Create, delete, duplicate, and merge layers
- **Layer Properties**: Visibility, opacity, blending modes
- **Layer Transformations**: Position, scale, and rotation
- **Layer Stack**: Z-order management of layers

### Tool System
The tool system provides an extensible framework for editing tools:

- **Selection Tool**: Rectangle selection functionality
- **Brush Tool**: Painting with various brush types
- **Eraser Tool**: Erasing with brush-like functionality
- **Extensibility**: Easy addition of new tools

### Brush System
The brush system implements advanced painting capabilities:

- **Brush Dynamics**: Pressure and tilt sensitivity
- **Brush Types**: Round, square, and texture brushes
- **Stroke Rendering**: Smooth stroke generation and rendering

## 3D Modeling Tools

### Mesh Editing
Basic mesh editing capabilities:

- **Vertex Manipulation**: Move, add, and delete vertices
- **Face Operations**: Extrude, inset, and delete faces
- **Edge Operations**: Loop cut and bridge edges

### Viewport System
3D viewport navigation and rendering:

- **Camera Controls**: Orbit, pan, and zoom navigation
- **View Presets**: Front, side, top, and perspective views
- **Grid and Guides**: 3D grid and axis visualization

## Building and Running

### Requirements
- CMake 3.16 or higher
- C++17 compatible compiler (MSVC recommended)
- Vulkan SDK
- SDL3 library

### Building on Windows
1. Ensure Vulkan SDK is installed
2. Open "Developer Command Prompt for VS 2022"
3. Navigate to the project directory
4. Run the build script:
   ```
   build_creative_suite.bat
   ```

### Running
After building, run the application using:
```
run_creative_suite.bat
```

Or directly execute:
```
build\Release\AstralCreativeSuite.exe
```

## Contributing

We welcome contributions to the Astral Creative Suite. Please follow these guidelines:

1. Fork the repository
2. Create a feature branch
3. Implement your changes
4. Add appropriate tests
5. Submit a pull request

## License

This project is licensed under the MIT License. See the LICENSE file for details.