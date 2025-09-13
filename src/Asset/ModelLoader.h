#pragma once

#include "Renderer/Model.h" // For Vertex struct
#include <string>
#include <vector>
#include <memory>

namespace AstralEngine {

    // A struct to hold the raw data loaded from a model file.
    struct ModelData {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        std::vector<SubMesh> subMeshes;
        // We'll expand this with material information later.
    };

    class ModelLoader {
    public:
        // Loads a model file from the given path and returns the raw data.
        // This is a static function as the loader itself doesn't need to maintain state.
        static std::unique_ptr<ModelData> loadModel(const std::string& filepath);
    };
}
