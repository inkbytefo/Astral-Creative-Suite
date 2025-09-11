#include "RenderSystem.h"
#include "Platform/Window.h"
#include "Core/Logger.h"
#include "Core/EngineConfig.h"
#include <stdexcept>

namespace AstralEngine {

    class RenderSystem::RenderSystemImpl {
    public:
        explicit RenderSystemImpl(Window& window) : m_window(window) {
            m_context = std::make_unique<Vulkan::VulkanContext>();
            m_initialized = false;
            m_framebufferResized = false;
        }

        ~RenderSystemImpl() {
            cleanup();
        }

        bool initialize() {
            try {
                AE_INFO("Initializing RenderSystem...");
                
                // Initialize Vulkan context
                createInstance();
                setupDebugMessenger();
                createSurface();
                m_context->device = std::make_unique<Vulkan::VulkanDevice>(m_context->instance, m_context->surface);
                m_context->swapChain = std::make_unique<Vulkan::VulkanSwapChain>(*m_context->device, m_window);
                
                // Initialize 2D and 3D renderers
                m_canvasRenderer = std::make_unique<D2::CanvasRenderer>(*m_context);
                m_sceneRenderer = std::make_unique<D3::SceneRenderer>(*m_context);
                
                m_initialized = true;
                AE_INFO("RenderSystem successfully initialized");
                return true;
            } catch (const std::exception& e) {
                AE_ERROR("Failed to initialize RenderSystem: {}", e.what());
                cleanup();
                return false;
            }
        }

        void cleanup() {
            if (!m_context || !m_context->device) {
                return;
            }

            vkDeviceWaitIdle(m_context->device->getDevice());
            
            // Clean up renderers
            m_canvasRenderer.reset();
            m_sceneRenderer.reset();
            
            // Clean up Vulkan resources
            m_context->swapChain.reset();
            m_context->device.reset();

            if (m_context->debugMessenger != VK_NULL_HANDLE) {
                auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(m_context->instance, "vkDestroyDebugUtilsMessengerEXT");
                if (func != nullptr) {
                    func(m_context->instance, m_context->debugMessenger, nullptr);
                }
                m_context->debugMessenger = VK_NULL_HANDLE;
            }

            if (m_context->surface != VK_NULL_HANDLE) {
                vkDestroySurfaceKHR(m_context->instance, m_context->surface, nullptr);
                m_context->surface = VK_NULL_HANDLE;
            }
            if (m_context->instance != VK_NULL_HANDLE) {
                vkDestroyInstance(m_context->instance, nullptr);
                m_context->instance = VK_NULL_HANDLE;
            }
        }

        void beginFrame() {
            if (!m_initialized) {
                AE_ERROR("RenderSystem not initialized, cannot begin frame");
                return;
            }
            
            // Frame initialization logic would go here
        }

        void endFrame() {
            if (!m_initialized) {
                AE_ERROR("RenderSystem not initialized, cannot end frame");
                return;
            }
            
            // Frame finalization logic would go here
        }

        void onFramebufferResize() {
            AE_DEBUG("Framebuffer resize requested");
            m_framebufferResized = true;
        }

        bool isFramebufferResized() const {
            return m_framebufferResized;
        }

        Vulkan::VulkanDevice& getDevice() {
            return *m_context->device;
        }

        const Vulkan::VulkanDevice& getDevice() const {
            return *m_context->device;
        }

        D2::CanvasRenderer& getCanvasRenderer() {
            return *m_canvasRenderer;
        }

        const D2::CanvasRenderer& getCanvasRenderer() const {
            return *m_canvasRenderer;
        }

        D3::SceneRenderer& getSceneRenderer() {
            return *m_sceneRenderer;
        }

        const D3::SceneRenderer& getSceneRenderer() const {
            return *m_sceneRenderer;
        }

        bool isInitialized() const {
            return m_initialized;
        }

        void printDebugInfo() const {
            AE_INFO("=== RenderSystem Debug Info ===");
            AE_INFO("Initialized: {}", m_initialized);
            AE_INFO("Framebuffer Resized: {}", m_framebufferResized);
            
            if (m_context && m_context->swapChain) {
                auto extent = m_context->swapChain->getSwapChainExtent();
                AE_INFO("Swap Chain Images: {}", m_context->swapChain->getImageCount());
                AE_INFO("Swap Chain Extent: {}x{}", extent.width, extent.height);
            }
            
            AE_INFO("=== End RenderSystem Debug Info ===");
        }

        void logMemoryUsage() const {
            // Memory usage logging would go here
        }

    private:
        void createInstance() {
            VkApplicationInfo appInfo{};
            appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            appInfo.pApplicationName = "Astral Creative Suite";
            appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.pEngineName = "Astral Engine";
            appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.apiVersion = VK_MAKE_VERSION(1, 4, 0);

            VkInstanceCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            createInfo.pApplicationInfo = &appInfo;

            // Get required instance extensions from SDL
            unsigned int count = 0;
            auto extensions_c = SDL_Vulkan_GetInstanceExtensions(&count);
            if (extensions_c == nullptr) {
                throw std::runtime_error("Failed to get SDL Vulkan extensions!");
            }
            std::vector<const char*> extensions(extensions_c, extensions_c + count);
            
            // Add debug messenger extension
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

            createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
            createInfo.ppEnabledExtensionNames = extensions.data();

            // Enable validation layers
            const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            if (vkCreateInstance(&createInfo, nullptr, &m_context->instance) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create Vulkan instance!");
            }
            AE_INFO("Vulkan instance created successfully.");
        }

        void setupDebugMessenger() {
            VkDebugUtilsMessengerCreateInfoEXT createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
                                        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
                                        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
                                    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
                                    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            createInfo.pfnUserCallback = debugCallback;

            auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(m_context->instance, "vkCreateDebugUtilsMessengerEXT");
            if (func != nullptr) {
                if (func(m_context->instance, &createInfo, nullptr, &m_context->debugMessenger) != VK_SUCCESS) {
                    throw std::runtime_error("Failed to setup debug messenger!");
                }
                AE_INFO("Vulkan debug messenger setup successfully.");
            } else {
                throw std::runtime_error("Debug messenger extension not found!");
            }
        }

        void createSurface() {
            if (!SDL_Vulkan_CreateSurface(m_window.getNativeWindow(), m_context->instance, nullptr, &m_context->surface)) {
                throw std::runtime_error("Failed to create Vulkan surface!");
            }
            AE_INFO("Vulkan surface created successfully.");
        }

        // Debug callback function
        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData) {

            if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
                AE_ERROR("Validation Layer: {}", pCallbackData->pMessage);
            } else {
                AE_WARN("Validation Layer: {}", pCallbackData->pMessage);
            }
            return VK_FALSE;
        }

        Window& m_window;
        std::unique_ptr<Vulkan::VulkanContext> m_context;
        std::unique_ptr<D2::CanvasRenderer> m_canvasRenderer;
        std::unique_ptr<D3::SceneRenderer> m_sceneRenderer;
        
        bool m_initialized = false;
        bool m_framebufferResized = false;
    };

    // RenderSystem implementation
    RenderSystem::RenderSystem(Window& window) 
        : m_impl(std::make_unique<RenderSystemImpl>(window)) {}

    RenderSystem::~RenderSystem() = default;

    bool RenderSystem::initialize() {
        return m_impl->initialize();
    }

    bool RenderSystem::isInitialized() const {
        return m_impl->isInitialized();
    }

    void RenderSystem::beginFrame() {
        m_impl->beginFrame();
    }

    void RenderSystem::endFrame() {
        m_impl->endFrame();
    }

    D2::CanvasRenderer& RenderSystem::getCanvasRenderer() {
        return m_impl->getCanvasRenderer();
    }

    const D2::CanvasRenderer& RenderSystem::getCanvasRenderer() const {
        return m_impl->getCanvasRenderer();
    }

    D3::SceneRenderer& RenderSystem::getSceneRenderer() {
        return m_impl->getSceneRenderer();
    }

    const D3::SceneRenderer& RenderSystem::getSceneRenderer() const {
        return m_impl->getSceneRenderer();
    }

    void RenderSystem::onFramebufferResize() {
        m_impl->onFramebufferResize();
    }

    bool RenderSystem::isFramebufferResized() const {
        return m_impl->isFramebufferResized();
    }

    Vulkan::VulkanDevice& RenderSystem::getDevice() {
        return m_impl->getDevice();
    }

    const Vulkan::VulkanDevice& RenderSystem::getDevice() const {
        return m_impl->getDevice();
    }

    void RenderSystem::printDebugInfo() const {
        m_impl->printDebugInfo();
    }

    void RenderSystem::logMemoryUsage() const {
        m_impl->logMemoryUsage();
    }

} // namespace AstralEngine