#pragma once

#include <cstdint>
#include <string>

namespace AstralEngine {
    
    /**
     * Engine configuration settings for performance tuning and debugging
     */
    struct EngineConfig {
        // Rendering settings
        bool enableVSync = true;
        bool enableDebugLayers = true;
        uint32_t targetFrameRate = 60;
        uint32_t maxFrameRate = 144;
        
        // SwapChain management
        uint32_t swapChainRecreationTimeoutMs = 5000;
        bool enableBackgroundSwapChainRecreation = true;
        bool skipFramesDuringRecreation = true;
        
        // Error handling
        uint32_t maxConsecutiveRenderErrors = 100;
        uint32_t errorRecoveryDelayMs = 16; // ~60 FPS
        
        // Performance monitoring
        bool enablePerformanceMonitoring = true;
        float slowOperationThresholdMs = 100.0f;
        uint32_t performanceStatsInterval = 300; // frames
        
        // Threading
        uint32_t assetLoadingThreads = 4;
        bool enableAsyncAssetLoading = true;
        
        // Debug settings
        bool enableVulkanValidation = true;
        bool logSlowFrames = true;
        bool printPerformanceStats = false;
        
        // Frame limiting
        bool enableFrameRateLimiting = true;
        uint32_t frameTimeTargetMs = 16; // ~60 FPS
        
        // Memory settings
        bool enableMemoryTracking = false;
        uint32_t vulkanHeapSizeMB = 512;
        
        // Get singleton instance
        static EngineConfig& getInstance();
        
        // Load from file (future implementation)
        void loadFromFile(const std::string& filepath);
        void saveToFile(const std::string& filepath) const;
        
        // Validation
        bool validate() const;
        void applyRuntimeLimits();
        
    private:
        static EngineConfig s_instance;
    };
    
    // Convenience macros
    #define ENGINE_CONFIG() AstralEngine::EngineConfig::getInstance()
    
} // namespace AstralEngine
