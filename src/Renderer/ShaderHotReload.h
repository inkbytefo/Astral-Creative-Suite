#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <filesystem>
#include <chrono>
#include <memory>

// Forward declarations
namespace AstralEngine { 
    class Shader;
    namespace Vulkan { class VulkanDevice; }
}

namespace AstralEngine {

    // Callback for when a shader is reloaded
    using ShaderReloadCallback = std::function<void(const std::string& shaderPath, std::shared_ptr<Shader> newShader)>;

    // File modification time tracking
    struct FileWatchInfo {
        std::filesystem::file_time_type lastWriteTime;
        std::string filePath;
        bool isValid = true;
    };

    class ShaderHotReload {
    public:
        ShaderHotReload(Vulkan::VulkanDevice& device);
        ~ShaderHotReload();

        // Register a shader file for hot-reload monitoring
        void watchShader(const std::string& shaderPath, std::shared_ptr<Shader> shader);
        
        // Unregister a shader file from monitoring
        void unwatchShader(const std::string& shaderPath);
        
        // Set callback for when shaders are reloaded
        void setReloadCallback(ShaderReloadCallback callback) { m_reloadCallback = callback; }
        
        // Check for file changes and reload if needed
        void update();
        
        // Enable/disable hot reloading
        void setEnabled(bool enabled) { m_enabled = enabled; }
        bool isEnabled() const { return m_enabled; }
        
        // Get statistics
        size_t getWatchedFileCount() const { return m_watchedFiles.size(); }
        uint32_t getReloadCount() const { return m_reloadCount; }

    private:
        bool checkFileModified(const std::string& filePath, FileWatchInfo& watchInfo);
        std::shared_ptr<Shader> reloadShader(const std::string& shaderPath);
        void logReloadAttempt(const std::string& shaderPath, bool success, const std::string& error = "");

        Vulkan::VulkanDevice& m_device;
        std::unordered_map<std::string, FileWatchInfo> m_watchedFiles;
        std::unordered_map<std::string, std::shared_ptr<Shader>> m_shaders;
        ShaderReloadCallback m_reloadCallback;
        
        bool m_enabled = true;
        uint32_t m_reloadCount = 0;
        std::chrono::steady_clock::time_point m_lastCheck;
        std::chrono::milliseconds m_checkInterval{500}; // Check every 500ms
    };

    // RAII wrapper for shader hot-reload in development builds
    class DevModeShaderWatcher {
    public:
        DevModeShaderWatcher(ShaderHotReload& hotReload, const std::string& shaderPath, std::shared_ptr<Shader> shader);
        ~DevModeShaderWatcher();

        // Non-copyable but movable
        DevModeShaderWatcher(const DevModeShaderWatcher&) = delete;
        DevModeShaderWatcher& operator=(const DevModeShaderWatcher&) = delete;
        DevModeShaderWatcher(DevModeShaderWatcher&&) = default;
        DevModeShaderWatcher& operator=(DevModeShaderWatcher&&) = default;

    private:
        ShaderHotReload* m_hotReload;
        std::string m_shaderPath;
    };

    // Utility macros for development mode
    #ifdef AE_DEBUG
        #define AE_WATCH_SHADER(hotReload, path, shader) DevModeShaderWatcher AE_CONCAT(_watcher_, __LINE__)((hotReload), (path), (shader))
        #define AE_ENABLE_SHADER_HOT_RELOAD true
    #else
        #define AE_WATCH_SHADER(hotReload, path, shader) 
        #define AE_ENABLE_SHADER_HOT_RELOAD false
    #endif

    // Helper macro for unique variable names
    #define AE_CONCAT_IMPL(x, y) x##y
    #define AE_CONCAT(x, y) AE_CONCAT_IMPL(x, y)

} // namespace AstralEngine
