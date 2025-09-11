#include "2D/CanvasRenderer.h"
#include "Core/Logger.h"
#include "Renderer/Shader.h"
#include <stdexcept>

namespace AstralEngine {
    namespace D2 {

        class CanvasRenderer::CanvasRendererImpl {
        public:
            explicit CanvasRendererImpl(Vulkan::VulkanContext& context) 
                : m_context(context) {
                m_initialized = false;
            }

            ~CanvasRendererImpl() {
                cleanup();
            }

            bool initialize() {
                try {
                    AE_INFO("Initializing CanvasRenderer...");
                    
                    // Initialize 2D rendering resources
                    createDescriptorSetLayouts();
                    createPipeline();
                    createCommandBuffers();
                    
                    m_initialized = true;
                    AE_INFO("CanvasRenderer successfully initialized");
                    return true;
                } catch (const std::exception& e) {
                    AE_ERROR("Failed to initialize CanvasRenderer: {}", e.what());
                    cleanup();
                    return false;
                }
            }

            void cleanup() {
                if (!m_context.device) {
                    return;
                }

                VkDevice device = m_context.device->getDevice();
                
                // Cleanup Vulkan resources
                if (m_pipeline) {
                    // Pipeline cleanup would go here
                    m_pipeline.reset();
                }
                
                if (m_descriptorPool != VK_NULL_HANDLE) {
                    vkDestroyDescriptorPool(device, m_descriptorPool, nullptr);
                    m_descriptorPool = VK_NULL_HANDLE;
                }
                
                if (m_descriptorSetLayout != VK_NULL_HANDLE) {
                    vkDestroyDescriptorSetLayout(device, m_descriptorSetLayout, nullptr);
                    m_descriptorSetLayout = VK_NULL_HANDLE;
                }
                
                m_initialized = false;
            }

            void renderCanvas(const CanvasRenderData& canvasData, 
                            const std::vector<ECS::EntityID>& layers) {
                if (!m_initialized) {
                    AE_ERROR("CanvasRenderer not initialized, cannot render canvas");
                    return;
                }
                
                AE_DEBUG("Rendering canvas: {}x{}, {} layers", 
                        canvasData.width, canvasData.height, layers.size());
                
                // Clear with background color
                clearBackground(canvasData.backgroundColor);
                
                // Render layers in order
                for (const auto& layerId : layers) {
                    renderLayer(layerId);
                }
                
                // Render grid if enabled
                if (canvasData.showGrid) {
                    renderGrid(canvasData);
                }
            }

            void renderGrid(const CanvasRenderData& canvasData) {
                if (!m_initialized) {
                    AE_ERROR("CanvasRenderer not initialized, cannot render grid");
                    return;
                }
                
                AE_DEBUG("Rendering grid: {}px", canvasData.gridSize);
                // Grid rendering implementation would go here
            }

            void renderLayer(ECS::EntityID layerId) {
                if (!m_initialized) {
                    AE_ERROR("CanvasRenderer not initialized, cannot render layer");
                    return;
                }
                
                AE_DEBUG("Rendering layer: {}", layerId);
                // Layer rendering implementation would go here
            }

            void renderBrush(const glm::vec2& position, float size, const glm::vec4& color) {
                if (!m_initialized) {
                    AE_ERROR("CanvasRenderer not initialized, cannot render brush");
                    return;
                }
                
                AE_DEBUG("Rendering brush at ({}, {}) size {} color ({}, {}, {}, {})",
                        position.x, position.y, size, color.r, color.g, color.b, color.a);
                // Brush rendering implementation would go here
            }

            void setViewTransform(const glm::mat3& transform) {
                m_viewTransform = transform;
            }

            glm::mat3 getViewTransform() const {
                return m_viewTransform;
            }

            bool isInitialized() const {
                return m_initialized;
            }

        private:
            void createDescriptorSetLayouts() {
                // Create descriptor set layouts for 2D rendering
                VkDescriptorSetLayoutBinding uboLayoutBinding{};
                uboLayoutBinding.binding = 0;
                uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                uboLayoutBinding.descriptorCount = 1;
                uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

                VkDescriptorSetLayoutCreateInfo layoutInfo{};
                layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                layoutInfo.bindingCount = 1;
                layoutInfo.pBindings = &uboLayoutBinding;

                if (vkCreateDescriptorSetLayout(m_context.device->getDevice(), &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS) {
                    throw std::runtime_error("Failed to create descriptor set layout!");
                }
            }

            void createPipeline() {
                // Create rendering pipeline for 2D elements
                // This would involve creating a pipeline with appropriate shaders
                AE_DEBUG("Creating 2D rendering pipeline");
            }

            void createCommandBuffers() {
                // Create command buffers for 2D rendering
                AE_DEBUG("Creating 2D command buffers");
            }

            void clearBackground(const glm::vec4& color) {
                // Clear the framebuffer with the specified background color
                AE_DEBUG("Clearing background with color ({}, {}, {}, {})",
                        color.r, color.g, color.b, color.a);
            }

            Vulkan::VulkanContext& m_context;
            std::unique_ptr<void> m_pipeline; // Placeholder for actual pipeline
            VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
            VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
            glm::mat3 m_viewTransform = glm::mat3(1.0f);
            bool m_initialized = false;
        };

        // CanvasRenderer implementation
        CanvasRenderer::CanvasRenderer(Vulkan::VulkanContext& context)
            : m_impl(std::make_unique<CanvasRendererImpl>(context)) {}

        CanvasRenderer::~CanvasRenderer() = default;

        bool CanvasRenderer::initialize() {
            return m_impl->initialize();
        }

        bool CanvasRenderer::isInitialized() const {
            return m_impl->isInitialized();
        }

        void CanvasRenderer::renderCanvas(const CanvasRenderData& canvasData, 
                                        const std::vector<ECS::EntityID>& layers) {
            m_impl->renderCanvas(canvasData, layers);
        }

        void CanvasRenderer::renderGrid(const CanvasRenderData& canvasData) {
            m_impl->renderGrid(canvasData);
        }

        void CanvasRenderer::renderLayer(ECS::EntityID layerId) {
            m_impl->renderLayer(layerId);
        }

        void CanvasRenderer::renderBrush(const glm::vec2& position, float size, const glm::vec4& color) {
            m_impl->renderBrush(position, size, color);
        }

        void CanvasRenderer::setViewTransform(const glm::mat3& transform) {
            m_impl->setViewTransform(transform);
        }

        glm::mat3 CanvasRenderer::getViewTransform() const {
            return m_impl->getViewTransform();
        }

        void CanvasRenderer::cleanup() {
            m_impl->cleanup();
        }

    } // namespace D2
} // namespace AstralEngine