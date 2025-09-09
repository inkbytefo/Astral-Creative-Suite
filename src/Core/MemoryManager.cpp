#include "MemoryManager.h"
#include "Logger.h"
#include <iomanip>
#include <sstream>

namespace AstralEngine::Memory {

void MemoryManager::printMemoryStats() const {
    AE_INFO("=== Memory Manager Statistics ===");
    
    size_t totalUsed = 0;
    size_t totalCapacity = 0;
    
    for (const auto* allocator : m_allocators) {
        size_t used = allocator->getUsedMemory();
        size_t total = allocator->getTotalMemory();
        float utilization = total > 0 ? (static_cast<float>(used) / total) * 100.0f : 0.0f;
        
        AE_INFO("{}: {:.2f} MB / {:.2f} MB ({:.1f}%)", 
                allocator->getName(),
                used / (1024.0f * 1024.0f),
                total / (1024.0f * 1024.0f),
                utilization);
        
        totalUsed += used;
        totalCapacity += total;
    }
    
    float overallUtilization = totalCapacity > 0 ? (static_cast<float>(totalUsed) / totalCapacity) * 100.0f : 0.0f;
    AE_INFO("Total: {:.2f} MB / {:.2f} MB ({:.1f}%)", 
            totalUsed / (1024.0f * 1024.0f),
            totalCapacity / (1024.0f * 1024.0f),
            overallUtilization);
    AE_INFO("==================================");
}

} // namespace AstralEngine::Memory
