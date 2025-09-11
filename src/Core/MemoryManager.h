#pragma once

#include <cstddef>

namespace AstralEngine {
    namespace Memory {
        class MemoryManager {
        public:
            static MemoryManager& getInstance() {
                static MemoryManager instance;
                return instance;
            }
            
            void initialize(size_t frameSizeMB, size_t stackSizeMB) {
                // Initialize memory manager with specified sizes
            }
            
            void shutdown() {
                // Shutdown memory manager
            }
            
            void newFrame() {
                // Reset frame allocator for new frame
            }
            
            class FrameAllocator {
            public:
                size_t getUsedMemory() const { return 0; }
                size_t getTotalMemory() const { return 0; }
            };
            
            class StackAllocator {
            public:
                size_t getUsedMemory() const { return 0; }
                size_t getTotalMemory() const { return 0; }
            };
            
            FrameAllocator* getFrameAllocator() { return nullptr; }
            StackAllocator* getStackAllocator() { return nullptr; }
        };
    }
}