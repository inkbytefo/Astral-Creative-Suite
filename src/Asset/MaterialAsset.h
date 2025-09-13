#pragma once

#include "Asset/Asset.h"
#include "Renderer/VulkanR/VulkanDevice.h"
#include <glm/glm.hpp>

namespace AstralEngine {

    class MaterialAsset : public Asset {
    public:
        MaterialAsset(const std::string& path);
        virtual ~MaterialAsset();
        
        virtual bool load() override;
        virtual void unload() override;
        virtual bool isLoaded() const override;
        
        // Material properties
        const glm::vec3& getAlbedo() const { return m_albedo; }
        float getMetallic() const { return m_metallic; }
        float getRoughness() const { return m_roughness; }
        const glm::vec3& getEmissive() const { return m_emissive; }
        
        void setAlbedo(const glm::vec3& albedo) { m_albedo = albedo; }
        void setMetallic(float metallic) { m_metallic = metallic; }
        void setRoughness(float roughness) { m_roughness = roughness; }
        void setEmissive(const glm::vec3& emissive) { m_emissive = emissive; }
        
    private:
        glm::vec3 m_albedo{1.0f};
        float m_metallic = 0.0f;
        float m_roughness = 0.5f;
        glm::vec3 m_emissive{0.0f};
        
        bool m_isLoaded;
    };

}