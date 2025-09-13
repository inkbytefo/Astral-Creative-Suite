#ifndef ASTRAL_ENGINE_MEMORY_MANAGER_H
#define ASTRAL_ENGINE_MEMORY_MANAGER_H

#include <cstddef>
#include <memory>
#include <mutex>

namespace AstralEngine {
    namespace Memory {
        class MemoryManager {
        public:
            static MemoryManager& getInstance() {
                static MemoryManager instance;
                return instance;
            }
            
            void initialize(size_t frameSizeMB, size_t stackSizeMB);
            void shutdown();
            void newFrame();
            
            class FrameAllocator {
            public:
                FrameAllocator(size_t size);
                ~FrameAllocator();
                
                void* allocate(size_t size);
                void reset();
                size_t getUsedMemory() const;
                size_t getTotalMemory() const;
                
            private:
                std::unique_ptr<uint8_t[]> m_memory;
                size_t m_totalSize;
                size_t m_usedSize;
                std::mutex m_mutex;
            };
            
            class StackAllocator {
            public:
                StackAllocator(size_t size);
                ~StackAllocator();
                
                void* allocate(size_t size);
                void* allocateAligned(size_t size, size_t alignment);
                void reset();
                size_t getUsedMemory() const;
                size_t getTotalMemory() const;
                
            private:
                std::unique_ptr<uint8_t[]> m_memory;
                size_t m_totalSize;
                size_t m_usedSize;
                std::mutex m_mutex;
            };
            
            FrameAllocator* getFrameAllocator();
            StackAllocator* getStackAllocator();
            
        private:
            MemoryManager() = default;
            ~MemoryManager() = default;
            
            std::unique_ptr<FrameAllocator> m_frameAllocator;
            std::unique_ptr<StackAllocator> m_stackAllocator;
            bool m_initialized;
            std::mutex m_mutex;
        };
    }
}

#endif // ASTRAL_ENGINE_MEMORY_MANAGER_H
