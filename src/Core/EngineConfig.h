#pragma once

namespace AstralEngine {
    struct EngineConfig {
        // Rendering configuration
        bool enableFrameRateLimiting = true;
        int targetFrameRate = 60;
        int maxFrameRate = 240;
        int frameTimeTargetMs = 16; // 1000ms / 60fps
        
        // Error handling
        int maxConsecutiveRenderErrors = 100;
        int errorRecoveryDelayMs = 100;
        
        // Performance monitoring
        bool enablePerformanceMonitoring = false;
        float slowOperationThresholdMs = 16.67f; // 1 frame at 60fps
        bool printPerformanceStats = true;
        int performanceStatsInterval = 60; // Print stats every 60 frames
        
        // Swap chain
        bool enableBackgroundSwapChainRecreation = true;
        bool skipFramesDuringRecreation = true;
        
        // Logging
        bool enableDetailedLogging = false;
        
        // Get singleton instance
        static EngineConfig& getInstance() {
            static EngineConfig instance;
            return instance;
        }
        
        // Apply runtime limits
        void applyRuntimeLimits() {
            // Apply any runtime configuration limits
        }
    };
    
    // Convenience macro
    #define ENGINE_CONFIG() ::AstralEngine::EngineConfig::getInstance()
}