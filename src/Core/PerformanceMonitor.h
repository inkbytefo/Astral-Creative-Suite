#ifndef ASTRAL_ENGINE_PERFORMANCE_MONITOR_H
#define ASTRAL_ENGINE_PERFORMANCE_MONITOR_H

#include <string>
#include <chrono>
#include <unordered_map>
#include <vector>
#include <mutex>

namespace AstralEngine {
    class PerformanceMonitor {
    public:
        struct PerformanceData {
            uint64_t callCount = 0;
            double totalTimeMs = 0.0;
            double averageTimeMs = 0.0;
            double minTimeMs = 0.0;
            double maxTimeMs = 0.0;
        };
        
        static void setSlowOperationThreshold(float thresholdMs);
        static void printStats();
        static void reset();
        
        // Performance measurement functions
        static void beginMeasurement(const std::string& operationName);
        static void endMeasurement(const std::string& operationName);
        
        static PerformanceData getPerformanceData(const std::string& operationName);
        static std::unordered_map<std::string, PerformanceData> getAllPerformanceData();
        
    private:
        static float s_slowOperationThreshold;
        static std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> s_startTimes;
        static std::unordered_map<std::string, PerformanceData> s_performanceData;
        static std::mutex s_mutex;
    };
}

#endif // ASTRAL_ENGINE_PERFORMANCE_MONITOR_H
