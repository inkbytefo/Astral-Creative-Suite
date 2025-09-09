#include "PerformanceMonitor.h"
#include "Logger.h"
#include <mutex>

namespace AstralEngine {
    
    // Static member definitions
    std::unordered_map<std::string, float> PerformanceMonitor::s_avgTimes;
    std::unordered_map<std::string, int> PerformanceMonitor::s_callCounts;
    std::atomic<float> PerformanceMonitor::s_slowThreshold{100.0f}; // 100ms default threshold
    std::mutex PerformanceMonitor::s_mutex;
    
    void PerformanceMonitor::trackFrameTime(const std::string& operation, float timeMs) {
        trackOperation(operation, timeMs);
    }
    
    void PerformanceMonitor::trackOperation(const std::string& operation, float timeMs) {
        std::lock_guard<std::mutex> lock(s_mutex);
        
        // Update running average
        float& avgTime = s_avgTimes[operation];
        int& callCount = s_callCounts[operation];
        
        if (callCount == 0) {
            avgTime = timeMs;
        } else {
            // Exponential moving average with more weight on recent values
            avgTime = avgTime * 0.9f + timeMs * 0.1f;
        }
        callCount++;
        
        // Log slow operations
        if (timeMs > s_slowThreshold.load()) {
            AE_WARN("Slow operation '{}': {:.2f}ms (avg: {:.2f}ms, calls: {})", 
                   operation, timeMs, avgTime, callCount);
        }
    }
    
    void PerformanceMonitor::reset() {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_avgTimes.clear();
        s_callCounts.clear();
    }
    
    void PerformanceMonitor::printStats() {
        std::lock_guard<std::mutex> lock(s_mutex);
        
        AE_INFO("=== Performance Statistics ===");
        for (const auto& [operation, avgTime] : s_avgTimes) {
            int callCount = s_callCounts[operation];
            AE_INFO("  {}: {:.2f}ms avg ({} calls)", operation, avgTime, callCount);
        }
        AE_INFO("==============================");
    }
    
    void PerformanceMonitor::setSlowOperationThreshold(float thresholdMs) {
        s_slowThreshold.store(thresholdMs);
        AE_DEBUG("Performance threshold set to {:.1f}ms", thresholdMs);
    }
    
    // Timer implementation
    PerformanceMonitor::Timer::Timer(const std::string& operation) 
        : m_operation(operation)
        , m_start(std::chrono::high_resolution_clock::now()) {
    }
    
    PerformanceMonitor::Timer::~Timer() {
        auto end = std::chrono::high_resolution_clock::now();
        float duration = std::chrono::duration<float, std::milli>(end - m_start).count();
        PerformanceMonitor::trackOperation(m_operation, duration);
    }
    
} // namespace AstralEngine
