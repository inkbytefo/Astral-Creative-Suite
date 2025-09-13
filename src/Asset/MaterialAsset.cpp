#include "MaterialAsset.h"
#include "Core/Logger.h"
#include <fstream>
#include <sstream>

namespace AstralEngine {

    MaterialAsset::MaterialAsset(const std::string& path)
        : Asset(path), m_albedo(1.0f), m_metallic(0.0f), m_roughness(0.5f), 
          m_emissive(0.0f), m_isLoaded(false) {
    }

    MaterialAsset::~MaterialAsset() {
        unload();
    }

    bool MaterialAsset::load() {
        // In a real implementation, this would load from a material file (e.g., JSON, MTL, etc.)
        // For now, we'll just use default values and mark as loaded
        
        // Example of what a real implementation might look like:
        /*
        std::ifstream file(getPath());
        if (!file.is_open()) {
            AE_ERROR("Failed to open material file: {}", getPath());
            return false;
        }
        
        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string key;
            iss >> key;
            
            if (key == "albedo") {
                iss >> m_albedo.x >> m_albedo.y >> m_albedo.z;
            } else if (key == "metallic") {
                iss >> m_metallic;
            } else if (key == "roughness") {
                iss >> m_roughness;
            } else if (key == "emissive") {
                iss >> m_emissive.x >> m_emissive.y >> m_emissive.z;
            }
        }
        */
        
        m_isLoaded = true;
        AE_INFO("Material asset loaded: {}", getPath());
        return true;
    }

    void MaterialAsset::unload() {
        m_isLoaded = false;
        AE_INFO("Material asset unloaded: {}", getPath());
    }

    bool MaterialAsset::isLoaded() const {
        return m_isLoaded;
    }

}