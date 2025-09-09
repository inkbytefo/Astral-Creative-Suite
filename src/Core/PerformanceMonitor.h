#pragma once

#include <string>
#include <unordered_map>
#include <chrono>
#include <atomic>
#include <mutex>

namespace AstralEngine {
    
    /**
     * Performance monitoring utility to track operation timing and detect slow operations
     */
    class PerformanceMonitor {
    public:
        static void trackFrameTime(const std::string& operation, float timeMs);
        static void trackOperation(const std::string& operation, float timeMs);
        static void reset();
        static void printStats();
        static void setSlowOperationThreshold(float thresholdMs);
        
        // RAII timer for easy usage
        class Timer {
        public:
            explicit Timer(const std::string& operation);
            ~Timer();
            
        private:
            std::string m_operation;
            std::chrono::high_resolution_clock::time_point m_start;
        };
        
    private:
        static std::unordered_map<std::string, float> s_avgTimes;
        static std::unordered_map<std::string, int> s_callCounts;
        static std::atomic<float> s_slowThreshold;
        static std::mutex s_mutex;
    };
    
    // Macro for easy timing
    #define PERF_TIMER(operation) AstralEngine::PerformanceMonitor::Timer _timer(operation)
    
} // namespace AstralEngine
