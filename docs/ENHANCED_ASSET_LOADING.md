# Enhanced Asset Loading and Dependency Management System

## Overview

The AstralEngine's asset loading system has been significantly improved to centralize material and texture loading within the `Model` class, using `MaterialLibrary` and `DependencyGraph` for better integration and automatic dependency management.

## Key Improvements

### 1. Centralized Material Management
- **Before**: Manual material assignment to `RenderComponent` after model loading
- **After**: Automatic material creation and binding within `Model::loadAsync()`

### 2. Automatic Texture Dependency Loading
- Models automatically parse MTL files and extract texture dependencies
- Textures are loaded asynchronously using `DependencyGraph`
- Materials are created with loaded textures and bound to submeshes

### 3. Simplified RenderComponent
- **Before**: Held both `Model` and `MaterialInstance` references
- **After**: Only holds `Model` reference; materials are accessed through the model

## Architecture

### Core Components

1. **Model Class** (`src/Renderer/Model.h/cpp`)
   - Manages geometry, materials, and texture dependencies
   - Provides async loading with progress tracking
   - Automatically binds materials to submeshes

2. **MaterialLibrary** (`src/Core/AssetDependency.h/cpp`) 
   - Parses MTL files into `MaterialInfo` structures
   - Creates `UnifiedMaterialInstance` objects with loaded textures
   - Handles texture path extraction and normalization

3. **DependencyGraph** (`src/Core/AssetDependency.h/cpp`)
   - Manages asynchronous texture loading
   - Provides progress tracking (0.0 to 1.0)
   - Handles failures gracefully

4. **RenderComponent** (`src/ECS/Components.h`)
   - Simplified to hold only a `Model` reference
   - Optional material override for special cases
   - Helper methods for accessing effective materials

### Data Flow

```
OBJ File Loading → MTL Parsing → Texture Dependencies → Async Loading → Material Creation → Submesh Binding
     ↓                ↓              ↓                    ↓               ↓                   ↓
  Geometry        MaterialInfo   Normalized Paths    DependencyGraph   UnifiedMaterial    Ready Model
```

## Usage Examples

### Basic Model Loading

```cpp
// Create and load a model
auto model = std::make_shared<Model>(device, "assets/models/chair.obj");

// Start async loading (materials and textures)
model->loadAsync([](bool success, const std::string& error) {
    if (success) {
        AE_INFO("Model loaded successfully");
    } else {
        AE_ERROR("Model loading failed: {}", error);
    }
});

// Create render component
auto renderComp = RenderComponent(model);
```

### Progress Tracking

```cpp
// Check loading progress
float progress = model->getMaterialLoadingProgress();  // 0.0 to 1.0
bool ready = model->areMaterialsReady();

// Get materials when ready
if (ready) {
    const auto& materials = model->getMaterials();
    auto submeshMaterial = model->getSubmeshMaterial(0);
}
```

### Advanced Usage with Material Override

```cpp
// Create custom material override
auto customMaterial = std::make_shared<UnifiedMaterialInstance>();
*customMaterial = UnifiedMaterialInstance::createPBRMaterial();
customMaterial->setBaseColor(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

// Apply to render component
auto renderComp = RenderComponent(model, customMaterial);

// Or access effective material per submesh
auto effectiveMat = renderComp.getEffectiveMaterial(submeshIndex);
```

## API Reference

### Model Class

#### Async Loading
```cpp
void loadAsync(LoadCompleteCallback callback);
bool areMaterialsReady() const;
float getMaterialLoadingProgress() const;
```

#### Material Access
```cpp
const std::unordered_map<std::string, std::shared_ptr<UnifiedMaterialInstance>>& getMaterials() const;
std::shared_ptr<UnifiedMaterialInstance> getMaterial(const std::string& name) const;
std::shared_ptr<UnifiedMaterialInstance> getSubmeshMaterial(uint32_t index) const;
std::shared_ptr<UnifiedMaterialInstance> getDefaultMaterial() const;
```

### RenderComponent Class

#### Model Management
```cpp
void setModel(std::shared_ptr<Model> m);
std::shared_ptr<Model> getModel() const;
```

#### Material Access
```cpp
std::shared_ptr<UnifiedMaterialInstance> getEffectiveMaterial(uint32_t submeshIndex = 0) const;
```

### DependencyGraph Class

#### Progress Tracking
```cpp
float getLoadingProgress() const;
bool isFullyLoaded() const;
```

## Migration Guide

### For Existing Code

**Before** (Manual material assignment):
```cpp
auto model = std::make_shared<Model>(device, filepath);
auto material = AssetManager::loadMaterial(device, materialPath);
auto renderComp = RenderComponent(model, material);
```

**After** (Automatic material loading):
```cpp
auto model = std::make_shared<Model>(device, filepath);
model->loadAsync([](bool success, const std::string& error) {
    // Handle completion
});
auto renderComp = RenderComponent(model);
```

### Key Changes

1. **Remove manual material loading** - Models handle this automatically
2. **Use Model-centric RenderComponent** - Pass only the model
3. **Handle async completion** - Materials are available after `loadAsync` completes
4. **Access materials through Model** - Use `getSubmeshMaterial()` instead of separate material references

## Error Handling

The system is designed to be robust:

- **Missing MTL files**: Model continues with default materials
- **Failed texture loads**: Materials are created without textures
- **Partial failures**: Loading continues with available resources
- **Timeout handling**: Graceful degradation to default materials

## Performance Considerations

- **Texture deduplication**: Same textures are loaded only once
- **Path normalization**: Prevents duplicate loads from different relative paths
- **Async loading**: Non-blocking texture and material loading
- **Progress tracking**: Enables loading screens and UI feedback

## File Structure

```
src/
├── Core/
│   ├── AssetDependency.h/cpp     # MaterialLibrary, DependencyGraph
│   └── AssetManager.h/cpp        # Enhanced with auto-dependencies
├── Renderer/
│   ├── Model.h/cpp               # Enhanced with material management
│   ├── UnifiedMaterial.h/cpp     # Material instance system
│   └── Texture.h/cpp             # Texture loading
└── ECS/
    └── Components.h              # Simplified RenderComponent
```

## Thread Safety

- Uses `std::atomic<bool>` for material ready state
- Dependency loading happens on background threads
- Materials are bound atomically after all dependencies complete
- Safe to query progress and readiness from any thread

## Future Enhancements

- Support for multiple MTL files per model
- Enhanced error recovery and retry mechanisms
- Streaming texture loading for large models
- Material template system for consistent appearance
