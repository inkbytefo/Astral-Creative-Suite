#ifndef ASTRAL_ENGINE_COMMON_H
#define ASTRAL_ENGINE_COMMON_H

#include "PerformanceMonitor.h"
#include "MemoryManager.h"

// Performance timer macro - automatically starts and stops timing for a scope
#define PERF_TIMER(name) \
    struct PerfTimer##__LINE__ { \
        const char* mName; \
        PerfTimer##__LINE__(const char* name) : mName(name) { \
            AstralEngine::PerformanceMonitor::beginMeasurement(name); \
        } \
        ~PerfTimer##__LINE__() { \
            AstralEngine::PerformanceMonitor::endMeasurement(mName); \
        } \
    } perfTimer##__LINE__(name);

// Stack allocator scope - automatically resets the stack allocator when leaving scope
#define STACK_SCOPE() \
    struct StackScope##__LINE__ { \
        StackScope##__LINE__() { \
            AstralEngine::Memory::MemoryManager::getInstance().getStackAllocator()->reset(); \
        } \
        ~StackScope##__LINE__() { \
            AstralEngine::Memory::MemoryManager::getInstance().getStackAllocator()->reset(); \
        } \
    } stackScope##__LINE__;

#endif // ASTRAL_ENGINE_COMMON_H