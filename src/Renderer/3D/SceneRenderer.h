#pragma once

#include "../Vulkan/VulkanContext.h"
#include "ECS/Scene.h"
#include "Renderer/UnifiedMaterial.h"
#include <memory>

namespace AstralEngine {
    namespace D3 {
        
        struct SceneRenderData {
            // Scene rendering parameters would go here
        };

        /**
         * @brief SceneRenderer handles 3D scene rendering operations
         * 
         * This class is responsible for rendering 3D scenes using the existing
         * Vulkan-based rendering pipeline, maintaining compatibility with the
         * current 3D rendering implementation.
         */
        class SceneRenderer {
        public:
            explicit SceneRenderer(Vulkan::VulkanContext& context);
            ~SceneRenderer();

            // Non-copyable but movable
            SceneRenderer(const SceneRenderer&) = delete;
            SceneRenderer& operator=(const SceneRenderer&) = delete;
            SceneRenderer(SceneRenderer&&) noexcept = default;
            SceneRenderer& operator=(SceneRenderer&&) noexcept = default;

            // Initialization
            bool initialize();
            bool isInitialized() const;

            // Scene rendering
            void renderScene(const ECS::Scene& scene);
            
            // Model rendering
            void renderModel(const std::shared_ptr<Model>& model, 
                           const std::shared_ptr<UnifiedMaterialInstance>& material,
                           const glm::mat4& transform);
            
            // Shadow rendering
            void renderShadows(const ECS::Scene& scene);
            
            // Post-processing
            void applyPostProcessing();
            
            // Resource management
            void cleanup();

        private:
            class SceneRendererImpl;
            std::unique_ptr<SceneRendererImpl> m_impl;
        };

    } // namespace D3
} // namespace AstralEngine