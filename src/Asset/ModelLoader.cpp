#include "Asset/ModelLoader.h"
#include "Core/Logger.h"
#include "Core/AssetLocator.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <unordered_map>
#include <filesystem>

namespace AstralEngine {

    std::unique_ptr<ModelData> ModelLoader::loadModel(const std::string& filepath) {
        auto resolvedPath = AssetLocator::getInstance().resolveAssetPath(filepath);
        if (resolvedPath.empty()) {
            AE_ERROR("Model file not found: {}", filepath);
            return nullptr;
        }

        std::filesystem::path modelPath(resolvedPath);
        std::string baseDirectory = modelPath.parent_path().string();

        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, resolvedPath.c_str(), baseDirectory.c_str(), true)) {
            AE_ERROR("Failed to load model '{}': {}", resolvedPath, err);
            return nullptr;
        }

        if (!warn.empty()) {
            AE_WARN("Warning while loading model '{}': {}", resolvedPath, warn);
        }

        auto modelData = std::make_unique<ModelData>();
        std::unordered_map<Vertex, uint32_t> uniqueVertices{};

        for (const auto& shape : shapes) {
            uint32_t indexOffset = static_cast<uint32_t>(modelData->indices.size());
            
            for (const auto& index : shape.mesh.indices) {
                Vertex vertex{};

                vertex.position = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };

                if (index.texcoord_index >= 0) {
                    vertex.texCoord = {
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                    };
                }

                if (index.normal_index >= 0) {
                    vertex.normal = {
                        attrib.normals[3 * index.normal_index + 0],
                        attrib.normals[3 * index.normal_index + 1],
                        attrib.normals[3 * index.normal_index + 2]
                    };
                }

                vertex.color = {1.0f, 1.0f, 1.0f};

                if (uniqueVertices.count(vertex) == 0) {
                    uniqueVertices[vertex] = static_cast<uint32_t>(modelData->vertices.size());
                    modelData->vertices.push_back(vertex);
                }
                modelData->indices.push_back(uniqueVertices[vertex]);
            }

            uint32_t indexCount = static_cast<uint32_t>(modelData->indices.size()) - indexOffset;
            if (indexCount > 0) {
                std::string materialName = "default";
                if (!shape.mesh.material_ids.empty() && shape.mesh.material_ids[0] >= 0) {
                    materialName = materials[shape.mesh.material_ids[0]].name;
                }
                modelData->subMeshes.emplace_back(shape.name, materialName, indexOffset, indexCount);
            }
        }
        
        // TODO: Process materials and add them to ModelData

        AE_INFO("Successfully loaded model data for '{}'. Vertices: {}, Indices: {}", filepath, modelData->vertices.size(), modelData->indices.size());
        return modelData;
    }
}