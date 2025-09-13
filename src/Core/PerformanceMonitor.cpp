#include "PerformanceMonitor.h"
#include "Logger.h"
#include <iostream>
#include <iomanip>
#include <sstream>

namespace AstralEngine {
    // Static member definitions
    float PerformanceMonitor::s_slowOperationThreshold = 16.67f; // 1 frame at 60fps
    std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> PerformanceMonitor::s_startTimes;
    std::unordered_map<std::string, PerformanceMonitor::PerformanceData> PerformanceMonitor::s_performanceData;
    std::mutex PerformanceMonitor::s_mutex;
    
    void PerformanceMonitor::setSlowOperationThreshold(float thresholdMs) {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_slowOperationThreshold = thresholdMs;
    }
    
    void PerformanceMonitor::printStats() {
        std::lock_guard<std::mutex> lock(s_mutex);
        AE_INFO("=== Performance Statistics ===");
        
        for (const auto& pair : s_performanceData) {
            const std::string& operationName = pair.first;
            const PerformanceData& data = pair.second;
            
            std::stringstream ss;
            ss << std::fixed << std::setprecision(2);
            ss << operationName << ": ";
            ss << "Calls: " << data.callCount << ", ";
            ss << "Total: " << data.totalTimeMs << "ms, ";
            ss << "Avg: " << data.averageTimeMs << "ms, ";
            ss << "Min: " << data.minTimeMs << "ms, ";
            ss << "Max: " << data.maxTimeMs << "ms";
            AE_INFO(ss.str());
            
            // Warn about slow operations
            if (data.averageTimeMs > s_slowOperationThreshold) {
                std::stringstream warn_ss;
                warn_ss << "Slow operation detected: " << operationName << " (avg: " << data.averageTimeMs << "ms)";
                AE_WARN(warn_ss.str());
            }
        }
    }
    
    void PerformanceMonitor::reset() {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_startTimes.clear();
        s_performanceData.clear();
    }
    
    void PerformanceMonitor::beginMeasurement(const std::string& operationName) {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_startTimes[operationName] = std::chrono::high_resolution_clock::now();
    }
    
    void PerformanceMonitor::endMeasurement(const std::string& operationName) {
        std::lock_guard<std::mutex> lock(s_mutex);
        auto it = s_startTimes.find(operationName);
        if (it == s_startTimes.end()) {
            return;
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - it->second);
        double durationMs = duration.count() / 1000.0;
        
        auto& data = s_performanceData[operationName];
        data.callCount++;
        data.totalTimeMs += durationMs;
        data.averageTimeMs = data.totalTimeMs / data.callCount;
        
        if (data.callCount == 1) {
            data.minTimeMs = durationMs;
            data.maxTimeMs = durationMs;
        } else {
            data.minTimeMs = std::min(data.minTimeMs, durationMs);
            data.maxTimeMs = std::max(data.maxTimeMs, durationMs);
        }
        
        s_startTimes.erase(it);
    }
    
    PerformanceMonitor::PerformanceData PerformanceMonitor::getPerformanceData(const std::string& operationName) {
        std::lock_guard<std::mutex> lock(s_mutex);
        auto it = s_performanceData.find(operationName);
        if (it != s_performanceData.end()) {
            return it->second;
        }
        return PerformanceData{};
    }
    
    std::unordered_map<std::string, PerformanceMonitor::PerformanceData> PerformanceMonitor::getAllPerformanceData() {
        std::lock_guard<std::mutex> lock(s_mutex);
        return s_performanceData;
    }
}
