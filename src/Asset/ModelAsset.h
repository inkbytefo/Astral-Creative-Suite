#pragma once

#include "Renderer/Model.h"
#include "Renderer/VulkanR/VulkanDevice.h"
#include <memory>
#include <string>
#include <vector>

namespace AstralEngine {
    class ModelAsset {
    public:
        ModelAsset(const std::string& path, Vulkan::VulkanDevice& device);
        ~ModelAsset();

        // Returns the GPU-side model. Can be nullptr if not loaded yet.
        std::shared_ptr<Model> getModel();

        // Asynchronously loads the model data from disk and creates the GPU resources.
        void load();

        bool isLoaded() const;
        const std::string& getPath() const;

    private:
        std::string m_path;
        std::shared_ptr<Model> m_model;
        bool m_isLoaded = false;
        Vulkan::VulkanDevice& m_device;  // Reference to device for creating GPU resources
        // We can add CPU-side data here later if needed (e.g., for physics)
        // std::vector<Vertex> m_vertices;
        // std::vector<uint32_t> m_indices;
    };
}
