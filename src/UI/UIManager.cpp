#include "UI/UIManager.h"
#include "Renderer/RRenderer.h"
#include "Platform/Window.h"
#include "Renderer/VulkanR/VulkanContext.h"
#include "Renderer/VulkanR/VulkanDevice.h"
#include "2D/Tools/Tool.h"
#include "2D/Tools/Brush.h"


// ImGui includes
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

namespace AstralEngine {
    namespace UI {
        UIManager::UIManager() { }
        UIManager::~UIManager() { }

        void UIManager::Initialize(AstralEngine::Window& window, Renderer& renderer, D2::ToolManager& toolManager) {
            if (m_initialized) {
                AE_WARN("UIManager is already initialized.");
                return;
            }

            AE_INFO("Initializing UIManager...");

            m_window = &window;
            m_renderer = &renderer;
            m_toolManager = &toolManager;

            // Create descriptor pool for ImGui
            VkDescriptorPoolSize pool_sizes[] = {
                { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
                { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
                { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
                { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
                { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
                { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
                { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
            };

            VkDescriptorPoolCreateInfo pool_info = {};
            pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            pool_info.maxSets = 1000;
            pool_info.poolSizeCount = std::size(pool_sizes);
            pool_info.pPoolSizes = pool_sizes;

            if (vkCreateDescriptorPool(m_renderer->GetDevice().getDevice(), &pool_info, nullptr, &m_imguiPool) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create ImGui descriptor pool.");
            }

            // Setup ImGui context
            IMGUI_CHECKVERSION();
            m_context = ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
            io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

            ImGui::StyleColorsDark();

            // Setup Platform/Renderer backends
            ImGui_ImplSDL3_InitForVulkan(m_window->getNativeWindow());
            
            Vulkan::VulkanContext& context = m_renderer->GetContext();
            ImGui_ImplVulkan_InitInfo init_info = {};
            init_info.Instance = context.instance.get();
            init_info.PhysicalDevice = context.device->getPhysicalDevice();
            init_info.Device = context.device->getDevice();
            init_info.Queue = context.device->getGraphicsQueue();
            init_info.DescriptorPool = m_imguiPool;
            init_info.MinImageCount = 3;
            init_info.ImageCount = 3;
            init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
            
            // Dynamic rendering için format bilgilerini ayarla
            init_info.ColorAttachmentFormat = context.swapChain->getSwapChainImageFormat();
            init_info.DepthAttachmentFormat = VK_FORMAT_UNDEFINED; // ImGui depth attachment kullanmıyor
            // MSAASamples zaten yukarıda ayarlandı

            ImGui_ImplVulkan_Init(&init_info, nullptr); // RenderPass yerine nullptr geç

            // Upload Fonts
            io.Fonts->AddFontDefault();
            context.device->executeCommands([&](VkCommandBuffer cmd) {
                ImGui_ImplVulkan_CreateFontsTexture(cmd);
            });
            ImGui_ImplVulkan_DestroyFontUploadObjects();

            m_initialized = true;
            AE_INFO("UIManager initialized successfully.");
        }

        void UIManager::Shutdown() {
            if (!m_initialized) return;
            AE_INFO("Shutting down UIManager...");

            vkDestroyDescriptorPool(m_renderer->GetDevice().getDevice(), m_imguiPool, nullptr);
            ImGui_ImplVulkan_Shutdown();
            ImGui_ImplSDL3_Shutdown();
            ImGui::DestroyContext(m_context);

            m_initialized = false;
            AE_INFO("UIManager shut down successfully.");
        }

void UIManager::BeginFrame() {
    // Clean up previous frame's draw data
    m_currentDrawData = nullptr;
    
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

        void UIManager::Render() {
            switch (m_appState) {
                case AppState::WelcomeScreen:
                    DrawWelcomeScreen();
                    break;
                case AppState::Editor2D:
                    Draw2DEditor();
                    break;
                case AppState::Editor3D:
                    Draw3DEditorPlaceholder();
                    break;
            }
        }

void UIManager::EndFrame() {
    ImGui::Render();

    // Store draw data for renderer integration
    m_currentDrawData = ImGui::GetDrawData();

    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

        void UIManager::HandleEvent(Event& event) {
            // ImGui_ImplSDL3_ProcessEvent is called in Window::pollEvents
        }

        void UIManager::SetAppState(AppState state) {
            m_appState = state;
        }

        AppState UIManager::GetAppState() const {
            return m_appState;
        }

        void UIManager::DrawWelcomeScreen() {
            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(400, 200));
            
            ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;

            ImGui::Begin("Welcome to Astral Creative Suite", nullptr, flags);

            ImGui::Text("Choose an editor to start:");
            ImGui::Spacing();

            if (ImGui::Button("2D Editor", ImVec2(-1, 50))) {
                SetAppState(AppState::Editor2D);
            }
            ImGui::Spacing();
            if (ImGui::Button("3D Editor", ImVec2(-1, 50))) {
                SetAppState(AppState::Editor3D);
            }

            ImGui::End();
        }

        void UIManager::Draw2DEditor() {
            SetupDockspace();

            // Draw our UI windows
            DrawToolbar();
            DrawPropertiesPanel();

            // The canvas itself is rendered in editor_main
        }

        void UIManager::Draw3DEditorPlaceholder() {
            SetupDockspace();
            ImGui::Begin("3D Editor");
            ImGui::Text("3D Editor is currently under development.");
            if (ImGui::Button("Back to Welcome Screen")) {
                SetAppState(AppState::WelcomeScreen);
            }
            ImGui::End();
        }

        void UIManager::DrawToolbar() {
            ImGui::Begin("Tools");

            if (ImGui::Button("Brush")) {
                m_toolManager->selectTool("Brush");
            }
            if (ImGui::Button("Eraser")) {
                m_toolManager->selectTool("Eraser");
            }
            // Add more tools here...

            ImGui::End();
        }

        void UIManager::DrawPropertiesPanel() {
            ImGui::Begin("Properties");

            Tool* activeTool = m_toolManager->getActiveTool();
            if (!activeTool) {
                ImGui::Text("No tool selected.");
                ImGui::End();
                return;
            }

            ImGui::Text("Selected Tool: %s", activeTool->getName().c_str());
            ImGui::Separator();

            // Specific properties for Brush or Eraser
            if (activeTool->getName() == "Brush" || activeTool->getName() == "Eraser") {
                auto* brushTool = dynamic_cast<D2::BrushTool2D*>(activeTool);
                if (brushTool) {
                    D2::BrushTool& brushProps = brushTool->getBrushSystem().getCurrentBrush();
                    
                    if (activeTool->getName() == "Brush") {
                        ImGui::Text("Brush Settings");
                        ImGui::ColorEdit4("Color", &brushProps.color.x);
                    }
                    ImGui::SliderFloat("Size", &brushProps.size, 1.0f, 100.0f);
                    ImGui::SliderFloat("Hardness", &brushProps.hardness, 0.0f, 1.0f);
                }
            }

            ImGui::End();
        }

        void UIManager::SetupDockspace() {
            static bool dockspaceOpen = true;
            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->Pos);
            ImGui::SetNextWindowSize(viewport->Size);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

            ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

            ImGui::Begin("DockSpace", &dockspaceOpen, window_flags);
            ImGui::PopStyleVar(3);

            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

            if (ImGui::BeginMenuBar()) {
                if (ImGui::BeginMenu("File")) {
                    if (ImGui::MenuItem("Back to Welcome Screen")) {
                        SetAppState(AppState::WelcomeScreen);
                    }
                    ImGui::Separator();
                    ImGui::MenuItem("New");
                    ImGui::MenuItem("Open");
                    ImGui::MenuItem("Save");
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }

            ImGui::End();
        }

    } // namespace UI
} // namespace AstralEngine
