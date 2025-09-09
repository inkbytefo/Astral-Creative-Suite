#include "EngineConfig.h"
#include "Logger.h"
#include <algorithm>
#include <thread>

namespace AstralEngine {
    
    // Static instance
    EngineConfig EngineConfig::s_instance;
    
    EngineConfig& EngineConfig::getInstance() {
        return s_instance;
    }
    
    bool EngineConfig::validate() const {
        bool valid = true;
        
        if (targetFrameRate == 0 || targetFrameRate > 1000) {
            AE_ERROR("Invalid targetFrameRate: {} (must be 1-1000)", targetFrameRate);
            valid = false;
        }
        
        if (maxFrameRate == 0 || maxFrameRate > 1000) {
            AE_ERROR("Invalid maxFrameRate: {} (must be 1-1000)", maxFrameRate);
            valid = false;
        }
        
        if (swapChainRecreationTimeoutMs == 0 || swapChainRecreationTimeoutMs > 60000) {
            AE_ERROR("Invalid swapChainRecreationTimeoutMs: {} (must be 1-60000)", swapChainRecreationTimeoutMs);
            valid = false;
        }
        
        if (maxConsecutiveRenderErrors == 0) {
            AE_ERROR("maxConsecutiveRenderErrors must be > 0");
            valid = false;
        }
        
        if (assetLoadingThreads == 0) {
            AE_ERROR("assetLoadingThreads must be > 0");
            valid = false;
        }
        
        return valid;
    }
    
    void EngineConfig::applyRuntimeLimits() {
        // Clamp values to reasonable ranges
        targetFrameRate = std::clamp(targetFrameRate, 1u, 1000u);
        maxFrameRate = std::clamp(maxFrameRate, 1u, 1000u);
        swapChainRecreationTimeoutMs = std::clamp(swapChainRecreationTimeoutMs, 100u, 60000u);
        maxConsecutiveRenderErrors = std::clamp(maxConsecutiveRenderErrors, 1u, 10000u);
        errorRecoveryDelayMs = std::clamp(errorRecoveryDelayMs, 1u, 1000u);
        slowOperationThresholdMs = std::clamp(slowOperationThresholdMs, 1.0f, 10000.0f);
        performanceStatsInterval = std::clamp(performanceStatsInterval, 1u, 10000u);
        frameTimeTargetMs = std::clamp(frameTimeTargetMs, 1u, 1000u);
        vulkanHeapSizeMB = std::clamp(vulkanHeapSizeMB, 64u, 16384u);
        
        // Limit asset loading threads to reasonable range
        uint32_t hardwareConcurrency = std::thread::hardware_concurrency();
        if (hardwareConcurrency > 0) {
            assetLoadingThreads = std::clamp(assetLoadingThreads, 1u, hardwareConcurrency * 2);
        } else {
            assetLoadingThreads = std::clamp(assetLoadingThreads, 1u, 8u);
        }
        
        // Ensure frame rate limits make sense
        if (targetFrameRate > maxFrameRate) {
            AE_WARN("targetFrameRate ({}) > maxFrameRate ({}), adjusting", targetFrameRate, maxFrameRate);
            targetFrameRate = maxFrameRate;
        }
        
        // Calculate frame time target from frame rate
        frameTimeTargetMs = 1000 / targetFrameRate;
        
        AE_INFO("Engine config validated and applied:");
        AE_INFO("  Target FPS: %u, Max FPS: %u", targetFrameRate, maxFrameRate);
        AE_INFO("  SwapChain timeout: %ums", swapChainRecreationTimeoutMs);
        AE_INFO("  Background SwapChain recreation: %s", enableBackgroundSwapChainRecreation ? "true" : "false");
        AE_INFO("  Asset loading threads: %u", assetLoadingThreads);
        AE_INFO("  Performance monitoring: %s", enablePerformanceMonitoring ? "true" : "false");
    }
    
    void EngineConfig::loadFromFile(const std::string& filepath) {
        // TODO: Implement config file loading (JSON/TOML/INI)
        AE_WARN("Config file loading not yet implemented: {}", filepath);
    }
    
    void EngineConfig::saveToFile(const std::string& filepath) const {
        // TODO: Implement config file saving
        AE_WARN("Config file saving not yet implemented: {}", filepath);
    }
    
} // namespace AstralEngine
