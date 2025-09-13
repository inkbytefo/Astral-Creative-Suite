#include "Asset/ModelAsset.h"
#include "Asset/ModelLoader.h"
#include "Core/Logger.h"
#include "Renderer/VulkanR/VulkanDevice.h" // Required for device reference

namespace AstralEngine {

    ModelAsset::ModelAsset(const std::string& path, Vulkan::VulkanDevice& device) : m_path(path), m_device(device) {}

    ModelAsset::~ModelAsset() {}

    std::shared_ptr<Model> ModelAsset::getModel() {
        return m_model;
    }

    void ModelAsset::load() {
        if (m_isLoaded) {
            return;
        }

        AE_INFO("Loading ModelAsset: {}", m_path);

        // Step 1: Load raw data from disk using ModelLoader
        std::unique_ptr<ModelData> modelData = ModelLoader::loadModel(m_path);

        if (!modelData) {
            AE_ERROR("Failed to load model data for asset: {}", m_path);
            return;
        }

        // Step 2: Create the GPU-side Model resource
        // Use the injected device reference instead of global hack
        m_model = std::make_shared<Model>(m_device, std::move(modelData));

        // TODO: Handle material loading asynchronously

        m_isLoaded = true;
        AE_INFO("Successfully loaded ModelAsset: {}", m_path);
    }

    bool ModelAsset::isLoaded() const {
        return m_isLoaded;
    }

    const std::string& ModelAsset::getPath() const {
        return m_path;
    }
}
