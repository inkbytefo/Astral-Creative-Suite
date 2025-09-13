#include "RRenderer.h"
#include "VulkanR/VulkanContext.h"
#include "VulkanR/VulkanDevice.h"
#include "VulkanR/VulkanSwapChain.h"
#include "Core/Logger.h"
#include "Platform/Window.h"
#include "ECS/Scene.h"

#include <stdexcept>
#include <array>

namespace AstralEngine {

    class Renderer::RendererImpl {
    public:
        explicit RendererImpl(Window& window) : m_window(window) {
            AE_INFO("Initializing Renderer with refactored architecture");
            InitVulkan();
            createSyncObjects();
            createCommandBuffers();
        }

        ~RendererImpl() {
            if (m_context) {
                vkDeviceWaitIdle(m_context->getDevice().getDevice());
                cleanupSyncObjects();
            }
            AE_INFO("Renderer implementation destroyed.");
        }

        void DrawScene(const ECS::Scene& scene, ImGuiDrawData* imguiData = nullptr) {
            try {
                uint32_t imageIndex;
                VkResult result = m_context->getSwapChain().acquireNextImage(&imageIndex, 
                    m_syncObjects[m_currentFrame].imageAvailableSemaphore);

                if (result == VK_ERROR_OUT_OF_DATE_KHR) {
                    RecreateSwapChain();
                    return;
                } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
                    throw std::runtime_error("Failed to acquire swap chain image!");
                }

                // Wait for the previous frame to finish
                if (m_imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
                    vkWaitForFences(m_context->getDevice().getDevice(), 1, &m_imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
                }
                
                // Mark the image as now being in use by this frame
                m_imagesInFlight[imageIndex] = m_syncObjects[m_currentFrame].inFlightFence;

                // Reset the fence
                vkResetFences(m_context->getDevice().getDevice(), 1, &m_syncObjects[m_currentFrame].inFlightFence);

                // Begin command buffer recording
                VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];
                vkResetCommandBuffer(cmd, 0);

                VkCommandBufferBeginInfo beginInfo{};
                beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                beginInfo.flags = 0; // Optional
                beginInfo.pInheritanceInfo = nullptr; // Optional

                if (vkBeginCommandBuffer(cmd, &beginInfo) != VK_SUCCESS) {
                    throw std::runtime_error("Failed to begin recording command buffer!");
                }

                // Record commands
                recordCommandBuffer(cmd, imageIndex, scene, imguiData);

                if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
                    throw std::runtime_error("Failed to record command buffer!");
                }

                // Submit command buffer
                VkSubmitInfo submitInfo{};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

                VkSemaphore waitSemaphores[] = { m_syncObjects[m_currentFrame].imageAvailableSemaphore };
                VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
                submitInfo.waitSemaphoreCount = 1;
                submitInfo.pWaitSemaphores = waitSemaphores;
                submitInfo.pWaitDstStageMask = waitStages;
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = &cmd;

                VkSemaphore signalSemaphores[] = { m_syncObjects[m_currentFrame].renderFinishedSemaphore };
                submitInfo.signalSemaphoreCount = 1;
                submitInfo.pSignalSemaphores = signalSemaphores;

                if (vkQueueSubmit(m_context->getDevice().getGraphicsQueue(), 1, &submitInfo, 
                                m_syncObjects[m_currentFrame].inFlightFence) != VK_SUCCESS) {
                    throw std::runtime_error("Failed to submit draw command buffer!");
                }

                // Present
                result = m_context->getSwapChain().presentImage(imageIndex, 
                    m_syncObjects[m_currentFrame].renderFinishedSemaphore);

                if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
                    RecreateSwapChain();
                } else if (result != VK_SUCCESS) {
                    throw std::runtime_error("Failed to present swap chain image!");
                }

                m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

            } catch (const std::exception& e) {
                AE_ERROR("Exception in DrawScene: {}", e.what());
            }
        }

        void RecreateSwapChain() {
            int width, height;
            SDL_GetWindowSizeInPixels(m_window.getNativeWindow(), &width, &height);
            
            // Wait for device to be idle
            vkDeviceWaitIdle(m_context->getDevice().getDevice());
            
            // TODO: Recreate swap chain with new dimensions
            // This is a placeholder implementation
            AE_INFO("Swap chain recreation requested ({}x{})", width, height);
        }
        
        VulkanR::VulkanDevice& getDevice() const {
            return m_context->getDevice();
        }

    private:
        void InitVulkan() {
            // Create Vulkan context
            VulkanR::VulkanContext::ContextCreateInfo contextCreateInfo;
            
            // Instance creation info
            contextCreateInfo.instanceCreateInfo.appInfo.appName = "Astral Creative Suite";
            contextCreateInfo.instanceCreateInfo.appInfo.appVersion = VK_MAKE_VERSION(1, 0, 0);
            contextCreateInfo.instanceCreateInfo.appInfo.engineName = "Astral Engine";
            contextCreateInfo.instanceCreateInfo.appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
            contextCreateInfo.instanceCreateInfo.appInfo.apiVersion = VK_API_VERSION_1_3;
            contextCreateInfo.instanceCreateInfo.enableValidationLayers = true;
            
            // Validation layers
            contextCreateInfo.instanceCreateInfo.validationLayers = {
                "VK_LAYER_KHRONOS_validation"
            };
            
            // Window info
            int width, height;
            SDL_GetWindowSizeInPixels(m_window.getNativeWindow(), &width, &height);
            contextCreateInfo.windowWidth = static_cast<uint32_t>(width);
            contextCreateInfo.windowHeight = static_cast<uint32_t>(height);
            contextCreateInfo.window = m_window.getNativeWindow();
            
            m_context = std::make_unique<VulkanR::VulkanContext>(contextCreateInfo);
        }

        void createSyncObjects() {
            m_syncObjects.resize(MAX_FRAMES_IN_FLIGHT);
            m_imagesInFlight.resize(m_context->getSwapChain().getImageCount(), VK_NULL_HANDLE);

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                VkSemaphoreCreateInfo semaphoreInfo{};
                semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

                VkFenceCreateInfo fenceInfo{};
                fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
                fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

                if (vkCreateSemaphore(m_context->getDevice().getDevice(), &semaphoreInfo, nullptr, 
                                    &m_syncObjects[i].imageAvailableSemaphore) != VK_SUCCESS ||
                    vkCreateSemaphore(m_context->getDevice().getDevice(), &semaphoreInfo, nullptr, 
                                    &m_syncObjects[i].renderFinishedSemaphore) != VK_SUCCESS ||
                    vkCreateFence(m_context->getDevice().getDevice(), &fenceInfo, nullptr, 
                                &m_syncObjects[i].inFlightFence) != VK_SUCCESS) {
                    throw std::runtime_error("Failed to create synchronization objects for a frame!");
                }
            }
        }

        void cleanupSyncObjects() {
            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                vkDestroySemaphore(m_context->getDevice().getDevice(), m_syncObjects[i].imageAvailableSemaphore, nullptr);
                vkDestroySemaphore(m_context->getDevice().getDevice(), m_syncObjects[i].renderFinishedSemaphore, nullptr);
                vkDestroyFence(m_context->getDevice().getDevice(), m_syncObjects[i].inFlightFence, nullptr);
            }
        }

        void createCommandBuffers() {
            m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.commandPool = m_context->getDevice().getCommandPool();
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandBufferCount = (uint32_t)m_commandBuffers.size();

            if (vkAllocateCommandBuffers(m_context->getDevice().getDevice(), &allocInfo, m_commandBuffers.data()) != VK_SUCCESS) {
                throw std::runtime_error("Failed to allocate command buffers!");
            }
        }

        void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, 
                                const ECS::Scene& scene, ImGuiDrawData* imguiData) {
            // This is a placeholder implementation
            // In a real implementation, this would record actual rendering commands
            
            // Begin rendering with dynamic rendering
            VkRenderingInfoKHR renderingInfo{};
            renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
            renderingInfo.renderArea.offset = {0, 0};
            renderingInfo.renderArea.extent = m_context->getSwapChain().getExtent();
            renderingInfo.layerCount = 1;
            renderingInfo.colorAttachmentCount = 1;
            
            VkRenderingAttachmentInfoKHR colorAttachment{};
            colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
            colorAttachment.imageView = m_context->getSwapChain().getImageViews()[imageIndex];
            colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
            colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachment.clearValue = {0.0f, 0.0f, 0.0f, 1.0f}; // Clear to black
            
            renderingInfo.pColorAttachments = &colorAttachment;
            
            vkCmdBeginRenderingKHR(commandBuffer, &renderingInfo);
            
            // TODO: Add actual rendering commands here
            // This would include:
            // - Setting viewport and scissor
            // - Binding pipelines
            // - Binding descriptor sets
            // - Drawing geometry
            
            vkCmdEndRenderingKHR(commandBuffer);
        }

        static const int MAX_FRAMES_IN_FLIGHT = 2;

        Window& m_window;
        std::unique_ptr<VulkanR::VulkanContext> m_context;
        
        std::vector<VkCommandBuffer> m_commandBuffers;
        std::vector<FrameSyncObjects> m_syncObjects;
        std::vector<VkFence> m_imagesInFlight;
        size_t m_currentFrame = 0;
    };

    Renderer::Renderer(Window& window) : m_impl(std::make_unique<RendererImpl>(window)) {}
    Renderer::~Renderer() = default;

    void Renderer::DrawScene(const ECS::Scene& scene, ImGuiDrawData* imguiData) { 
        m_impl->DrawScene(scene, imguiData); 
    }

    void Renderer::RecreateSwapChain() {
        m_impl->RecreateSwapChain();
    }
    
    VulkanR::VulkanDevice& Renderer::getDevice() const {
        return m_impl->getDevice();
    }

} // namespace AstralEngine