#pragma once

#include "ECS/Components.h"
#include "Renderer/Texture.h"
#include <string>
#include <memory>
#include <vector>

namespace AstralEngine {
    namespace D2 {
        // Blend modes for layers
        enum class BlendMode {
            Normal,
            Multiply,
            Screen,
            Overlay,
            Darken,
            Lighten,
            ColorDodge,
            ColorBurn,
            HardLight,
            SoftLight,
            Difference,
            Exclusion,
            Hue,
            Saturation,
            Color,
            Luminosity
        };
        
        /**
         * @brief Layer component for 2D graphics editing
         * 
         * This component represents a single layer in the 2D graphics editor.
         * It contains the layer's content, properties, and effects.
         */
        struct Layer : public ECS::IComponent {
            // Layer content
            std::shared_ptr<Texture> content;
            
            // Layer properties
            std::string name = "Layer";
            float opacity = 1.0f;
            bool visible = true;
            BlendMode blendMode = BlendMode::Normal;
            
            // Transform properties
            glm::vec2 position = {0, 0};
            glm::vec2 scale = {1, 1};
            float rotation = 0.0f;
            
            // Layer effects (to be implemented)
            // std::vector<std::shared_ptr<LayerEffect>> effects;
            
            // Constructor
            Layer() = default;
            
            // Constructor with name
            explicit Layer(const std::string& layerName) : name(layerName) {}
            
            // Helper methods
            bool isVisible() const { return visible && opacity > 0.0f; }
            glm::mat3 getTransformMatrix() const;
        };
        
        /**
         * @brief Layer manager system
         * 
         * This system manages the layer stack and provides operations for layer manipulation.
         */
        class LayerSystem {
        public:
            LayerSystem(ECS::Scene& scene);
            ~LayerSystem() = default;
            
            // Layer operations
            ECS::EntityID addLayer(const std::string& name);
            void removeLayer(ECS::EntityID layerId);
            void moveLayer(ECS::EntityID layerId, int newPosition);
            void duplicateLayer(ECS::EntityID layerId);
            
            // Layer stack operations
            void mergeLayers(ECS::EntityID layer1, ECS::EntityID layer2);
            void flattenLayers();
            
            // Layer selection
            void selectLayer(ECS::EntityID layerId);
            void deselectLayer(ECS::EntityID layerId);
            void clearSelection();
            const std::vector<ECS::EntityID>& getSelectedLayers() const { return m_selectedLayers; }
            
            // Layer stack access
            const std::vector<ECS::EntityID>& getLayerStack() const { return m_layerStack; }
            
            // Rendering
            void renderLayer(ECS::EntityID layerId);
            
        private:
            ECS::Scene& m_scene;
            std::vector<ECS::EntityID> m_layerStack;
            std::vector<ECS::EntityID> m_selectedLayers;
        };
    }
}