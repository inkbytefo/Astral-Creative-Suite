#include "ShaderHotReload.h"
#include "Shader.h"
#include "Renderer/Vulkan/VulkanDevice.h"
#include "Core/Logger.h"
#include <filesystem>

namespace AstralEngine {

ShaderHotReload::ShaderHotReload(Vulkan::VulkanDevice& device) 
    : m_device(device) {
    m_lastCheck = std::chrono::steady_clock::now();
    AE_INFO("Shader hot-reload system initialized");
}

ShaderHotReload::~ShaderHotReload() {
    AE_INFO("Shader hot-reload system shutdown. Total reloads: {}", m_reloadCount);
}

void ShaderHotReload::watchShader(const std::string& shaderPath, std::shared_ptr<Shader> shader) {
    if (!m_enabled) {
        return;
    }
    
    try {
        if (!std::filesystem::exists(shaderPath)) {
            AE_WARN("Shader file does not exist: {}", shaderPath);
            return;
        }
        
        FileWatchInfo watchInfo;
        watchInfo.filePath = shaderPath;
        watchInfo.lastWriteTime = std::filesystem::last_write_time(shaderPath);
        watchInfo.isValid = true;
        
        m_watchedFiles[shaderPath] = watchInfo;
        m_shaders[shaderPath] = shader;
        
        AE_INFO("Watching shader for hot-reload: {}", shaderPath);
    } catch (const std::filesystem::filesystem_error& e) {
        AE_ERROR("Failed to watch shader file {}: {}", shaderPath, e.what());
    }
}

void ShaderHotReload::unwatchShader(const std::string& shaderPath) {
    auto it = m_watchedFiles.find(shaderPath);
    if (it != m_watchedFiles.end()) {
        m_watchedFiles.erase(it);
        m_shaders.erase(shaderPath);
        AE_INFO("Stopped watching shader: {}", shaderPath);
    }
}

void ShaderHotReload::update() {
    if (!m_enabled) {
        return;
    }
    
    auto now = std::chrono::steady_clock::now();
    if (now - m_lastCheck < m_checkInterval) {
        return;
    }
    m_lastCheck = now;
    
    for (auto& [shaderPath, watchInfo] : m_watchedFiles) {
        if (checkFileModified(shaderPath, watchInfo)) {
            try {
                AE_INFO("Detected shader file change: {}", shaderPath);
                
                // Attempt to reload the shader
                auto newShader = reloadShader(shaderPath);
                if (newShader) {
                    m_shaders[shaderPath] = newShader;
                    m_reloadCount++;
                    
                    // Notify callback if set
                    if (m_reloadCallback) {
                        m_reloadCallback(shaderPath, newShader);
                    }
                    
                    logReloadAttempt(shaderPath, true);
                } else {
                    logReloadAttempt(shaderPath, false, "Failed to create new shader");
                }
            } catch (const std::exception& e) {
                logReloadAttempt(shaderPath, false, e.what());
                // Mark file as temporarily invalid to avoid spam
                watchInfo.isValid = false;
            }
        }
    }
}

bool ShaderHotReload::checkFileModified(const std::string& filePath, FileWatchInfo& watchInfo) {
    if (!watchInfo.isValid) {
        // Try to revalidate the file
        if (std::filesystem::exists(filePath)) {
            watchInfo.isValid = true;
        } else {
            return false;
        }
    }
    
    try {
        auto currentWriteTime = std::filesystem::last_write_time(filePath);
        if (currentWriteTime != watchInfo.lastWriteTime) {
            watchInfo.lastWriteTime = currentWriteTime;
            return true;
        }
    } catch (const std::filesystem::filesystem_error& e) {
        AE_WARN("Failed to check file modification time for {}: {}", filePath, e.what());
        watchInfo.isValid = false;
    }
    
    return false;
}

std::shared_ptr<Shader> ShaderHotReload::reloadShader(const std::string& shaderPath) {
    try {
        // Wait for Vulkan device to be idle before reloading
        vkDeviceWaitIdle(m_device.getDevice());
        
        // Create a new shader instance
        // Note: This assumes the Shader constructor takes a device and path
        // You might need to adapt this based on your Shader class interface
        auto newShader = std::make_shared<Shader>(m_device, shaderPath);
        
        return newShader;
    } catch (const std::exception& e) {
        AE_ERROR("Failed to reload shader {}: {}", shaderPath, e.what());
        return nullptr;
    }
}

void ShaderHotReload::logReloadAttempt(const std::string& shaderPath, bool success, const std::string& error) {
    if (success) {
        AE_INFO("Successfully reloaded shader: {}", shaderPath);
    } else {
        AE_ERROR("Failed to reload shader {}: {}", shaderPath, error);
    }
}

// DevModeShaderWatcher implementation
DevModeShaderWatcher::DevModeShaderWatcher(ShaderHotReload& hotReload, const std::string& shaderPath, std::shared_ptr<Shader> shader)
    : m_hotReload(&hotReload), m_shaderPath(shaderPath) {
    m_hotReload->watchShader(shaderPath, shader);
}

DevModeShaderWatcher::~DevModeShaderWatcher() {
    if (m_hotReload) {
        m_hotReload->unwatchShader(m_shaderPath);
    }
}

} // namespace AstralEngine
