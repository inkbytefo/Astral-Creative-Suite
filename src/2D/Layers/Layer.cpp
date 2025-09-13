#include "2D/Layers/Layer.h"
#include "Core/Logger.h"
#include "Renderer/RRenderer.h"
#include "Renderer/Texture.h"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

namespace AstralEngine {
    namespace D2 {
        glm::mat3 Layer::getTransformMatrix() const {
            // Create transformation matrix for 2D layer
            glm::mat3 translation = glm::translate(glm::mat3(1.0f), position);
            glm::mat3 rotationMat = glm::rotate(glm::mat3(1.0f), rotation);
            glm::mat3 scaleMat = glm::scale(glm::mat3(1.0f), scale);
            
            // Combine transformations: T * R * S
            return translation * rotationMat * scaleMat;
        }
        
        LayerSystem::LayerSystem(ECS::Scene& scene) : m_scene(scene) {
            AE_INFO("LayerSystem başlatıldı");
        }
        
        ECS::EntityID LayerSystem::addLayer(const std::string& name) {
            // Create a new entity for the layer
            ECS::EntityID layerId = m_scene.createEntity(name);
            
            // Add the layer component
            auto& layer = m_scene.addComponent<Layer>(layerId);
            layer.name = name;
            
            // Add to the layer stack at the top (end of vector)
            m_layerStack.push_back(layerId);
            
            AE_DEBUG("Layer eklendi: {} (ID: {})", name, layerId);
            return layerId;
        }
        
        void LayerSystem::removeLayer(ECS::EntityID layerId) {
            // Check if the layer exists
            if (!m_scene.hasComponent<Layer>(layerId)) {
                AE_WARN("Layer bulunamadı: {}", layerId);
                return;
            }
            
            // Remove from layer stack
            auto it = std::find(m_layerStack.begin(), m_layerStack.end(), layerId);
            if (it != m_layerStack.end()) {
                m_layerStack.erase(it);
            }
            
            // Remove from selection
            auto selIt = std::find(m_selectedLayers.begin(), m_selectedLayers.end(), layerId);
            if (selIt != m_selectedLayers.end()) {
                m_selectedLayers.erase(selIt);
            }
            
            // Destroy the entity
            m_scene.destroyEntity(layerId);
            
            AE_DEBUG("Layer silindi: {}", layerId);
        }
        
        void LayerSystem::moveLayer(ECS::EntityID layerId, int newPosition) {
            // Check if the layer exists
            if (!m_scene.hasComponent<Layer>(layerId)) {
                AE_WARN("Layer bulunamadı: {}", layerId);
                return;
            }
            
            // Find current position
            auto it = std::find(m_layerStack.begin(), m_layerStack.end(), layerId);
            if (it == m_layerStack.end()) {
                AE_WARN("Layer layer stack'te bulunamadı: {}", layerId);
                return;
            }
            
            // Calculate current index
            int currentIndex = std::distance(m_layerStack.begin(), it);
            
            // Clamp new position
            if (newPosition < 0) newPosition = 0;
            if (newPosition >= static_cast<int>(m_layerStack.size())) newPosition = m_layerStack.size() - 1;
            
            // No change needed
            if (currentIndex == newPosition) {
                return;
            }
            
            // Remove from current position
            m_layerStack.erase(it);
            
            // Insert at new position
            m_layerStack.insert(m_layerStack.begin() + newPosition, layerId);
            
            AE_DEBUG("Layer taşındı: {} from {} to {}", layerId, currentIndex, newPosition);
        }
        
        void LayerSystem::duplicateLayer(ECS::EntityID layerId) {
            // Check if the layer exists
            if (!m_scene.hasComponent<Layer>(layerId)) {
                AE_WARN("Layer bulunamadı: {}", layerId);
                return;
            }
            
            // Get the original layer
            const auto& originalLayer = m_scene.getComponent<Layer>(layerId);
            
            // Create a new layer with a modified name
            std::string newName = originalLayer.name + " Copy";
            ECS::EntityID newLayerId = addLayer(newName);
            
            // Copy properties
            auto& newLayer = m_scene.getComponent<Layer>(newLayerId);
            newLayer.opacity = originalLayer.opacity;
            newLayer.visible = originalLayer.visible;
            newLayer.blendMode = originalLayer.blendMode;
            newLayer.position = originalLayer.position;
            newLayer.scale = originalLayer.scale;
            newLayer.rotation = originalLayer.rotation;
            
            // TODO: Copy content (texture) when implemented
            // For now, we'll just copy the reference
            newLayer.content = originalLayer.content;
            
            AE_DEBUG("Layer kopyalandı: {} -> {}", layerId, newLayerId);
        }
        
        void LayerSystem::mergeLayers(ECS::EntityID layer1, ECS::EntityID layer2) {
            // Check if both layers exist
            if (!m_scene.hasComponent<Layer>(layer1) || !m_scene.hasComponent<Layer>(layer2)) {
                AE_WARN("Bir veya daha fazla layer bulunamadı: {}, {}", layer1, layer2);
                return;
            }
            
            // For now, we'll just remove the second layer
            // In a full implementation, we would actually merge the contents
            removeLayer(layer2);
            
            AE_DEBUG("Layer'lar birleştirildi: {} ve {}", layer1, layer2);
        }
        
        void LayerSystem::flattenLayers() {
            // For now, we'll just clear all layers except the first one
            // In a full implementation, we would actually merge all layers
            while (m_layerStack.size() > 1) {
                removeLayer(m_layerStack.back());
            }
            
            AE_DEBUG("Tüm layer'lar düzleştirildi");
        }
        
        void LayerSystem::selectLayer(ECS::EntityID layerId) {
            // Check if the layer exists
            if (!m_scene.hasComponent<Layer>(layerId)) {
                AE_WARN("Layer bulunamadı: {}", layerId);
                return;
            }
            
            // Add to selection if not already selected
            auto it = std::find(m_selectedLayers.begin(), m_selectedLayers.end(), layerId);
            if (it == m_selectedLayers.end()) {
                m_selectedLayers.push_back(layerId);
                AE_DEBUG("Layer seçildi: {}", layerId);
            }
        }
        
        void LayerSystem::deselectLayer(ECS::EntityID layerId) {
            auto it = std::find(m_selectedLayers.begin(), m_selectedLayers.end(), layerId);
            if (it != m_selectedLayers.end()) {
                m_selectedLayers.erase(it);
                AE_DEBUG("Layer seçimi kaldırıldı: {}", layerId);
            }
        }
        
        void LayerSystem::clearSelection() {
            m_selectedLayers.clear();
            AE_DEBUG("Tüm layer seçimleri kaldırıldı");
        }
        
        void LayerSystem::renderLayer(ECS::EntityID layerId) {
            // Check if the layer exists
            if (!m_scene.hasComponent<Layer>(layerId)) {
                AE_WARN("Layer bulunamadı: {}", layerId);
                return;
            }
            
            // Get the layer
            const auto& layer = m_scene.getComponent<Layer>(layerId);
            
            // Check if layer is visible
            if (!layer.isVisible()) {
                return;
            }
            
            // For now, we'll just log the operation
            // In a full implementation, we would render the layer using the renderer
            AE_DEBUG("Layer render ediliyor: {} (ID: {})", layer.name, layerId);
        }
    }
}