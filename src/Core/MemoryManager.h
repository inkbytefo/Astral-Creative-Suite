#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>
#include <bitset>
#include <atomic>
#include <mutex>
#include <cassert>

namespace AstralEngine::Memory {

    // Memory alignment utilities
    constexpr size_t CACHE_LINE_SIZE = 64;
    constexpr size_t DEFAULT_ALIGNMENT = alignof(std::max_align_t);

    inline size_t alignUp(size_t value, size_t alignment) {
        return (value + alignment - 1) & ~(alignment - 1);
    }

    inline void* alignPtr(void* ptr, size_t alignment) {
        uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
        uintptr_t aligned = alignUp(addr, alignment);
        return reinterpret_cast<void*>(aligned);
    }

    // Base allocator interface
    class IAllocator {
    public:
        virtual ~IAllocator() = default;
        virtual void* allocate(size_t size, size_t alignment = DEFAULT_ALIGNMENT) = 0;
        virtual void deallocate(void* ptr) = 0;
        virtual size_t getUsedMemory() const = 0;
        virtual size_t getTotalMemory() const = 0;
        virtual const char* getName() const = 0;
    };

    // Fixed-size object pool for fast allocation/deallocation
    template<typename T, size_t POOL_SIZE>
    class ObjectPool : public IAllocator {
    public:
        ObjectPool() {
            // Initialize free list
            for (size_t i = 0; i < POOL_SIZE - 1; ++i) {
                *reinterpret_cast<size_t*>(&m_pool[i * sizeof(T)]) = i + 1;
            }
            *reinterpret_cast<size_t*>(&m_pool[(POOL_SIZE - 1) * sizeof(T)]) = SIZE_MAX;
            m_freeHead = 0;
            m_allocatedCount = 0;
        }

        ~ObjectPool() = default;

        // Allocate object from pool
        template<typename... Args>
        T* allocate(Args&&... args) {
            std::lock_guard<std::mutex> lock(m_mutex);
            
            if (m_freeHead == SIZE_MAX) {
                return nullptr; // Pool exhausted
            }
            
            size_t index = m_freeHead;
            void* ptr = &m_pool[index * sizeof(T)];
            
            // Update free list
            m_freeHead = *reinterpret_cast<size_t*>(ptr);
            m_allocatedMask[index] = true;
            ++m_allocatedCount;
            
            // Construct object in place
            return new(ptr) T(std::forward<Args>(args)...);
        }

        // Deallocate object back to pool
        void deallocate(T* ptr) {
            std::lock_guard<std::mutex> lock(m_mutex);
            
            if (!ptr) return;
            
            // Calculate index
            uintptr_t poolStart = reinterpret_cast<uintptr_t>(m_pool);
            uintptr_t objAddr = reinterpret_cast<uintptr_t>(ptr);
            size_t index = (objAddr - poolStart) / sizeof(T);
            
            assert(index < POOL_SIZE);
            assert(m_allocatedMask[index]);
            
            // Destroy object
            ptr->~T();
            
            // Add to free list
            *reinterpret_cast<size_t*>(ptr) = m_freeHead;
            m_freeHead = index;
            m_allocatedMask[index] = false;
            --m_allocatedCount;
        }

        // IAllocator interface
        void* allocate(size_t size, size_t alignment = DEFAULT_ALIGNMENT) override {
            if (size != sizeof(T)) return nullptr;
            return allocate();
        }

        void deallocate(void* ptr) override {
            deallocate(static_cast<T*>(ptr));
        }

        size_t getUsedMemory() const override {
            return m_allocatedCount * sizeof(T);
        }

        size_t getTotalMemory() const override {
            return POOL_SIZE * sizeof(T);
        }

        const char* getName() const override {
            return "ObjectPool";
        }

        // Pool statistics
        size_t getUtilization() const {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_allocatedCount;
        }

        size_t getCapacity() const { return POOL_SIZE; }
        
        float getUtilizationRatio() const {
            return static_cast<float>(getUtilization()) / POOL_SIZE;
        }

    private:
        alignas(T) char m_pool[sizeof(T) * POOL_SIZE];
        std::bitset<POOL_SIZE> m_allocatedMask;
        size_t m_freeHead;
        std::atomic<size_t> m_allocatedCount;
        mutable std::mutex m_mutex;
    };

    // Linear allocator for frame-based allocations
    class LinearAllocator : public IAllocator {
    public:
        explicit LinearAllocator(size_t size, const char* name = "LinearAllocator")
            : m_buffer(nullptr), m_size(size), m_offset(0), m_name(name) {
#ifdef _WIN32
            m_buffer = static_cast<char*>(_aligned_malloc(size, CACHE_LINE_SIZE));
#else
            m_buffer = static_cast<char*>(std::aligned_alloc(CACHE_LINE_SIZE, size));
#endif
            assert(m_buffer && "Failed to allocate linear buffer");
        }

        ~LinearAllocator() {
            if (m_buffer) {
#ifdef _WIN32
                _aligned_free(m_buffer);
#else
                std::free(m_buffer);
#endif
            }
        }

        // Non-copyable
        LinearAllocator(const LinearAllocator&) = delete;
        LinearAllocator& operator=(const LinearAllocator&) = delete;

        // Movable
        LinearAllocator(LinearAllocator&& other) noexcept
            : m_buffer(other.m_buffer), m_size(other.m_size), 
              m_offset(other.m_offset.load()), m_name(other.m_name) {
            other.m_buffer = nullptr;
            other.m_size = 0;
            other.m_offset = 0;
        }

        // Allocate from linear buffer
        void* allocate(size_t size, size_t alignment = DEFAULT_ALIGNMENT) override {
            size_t currentOffset = m_offset.load(std::memory_order_relaxed);
            
            // Align the current offset
            uintptr_t aligned = alignUp(reinterpret_cast<uintptr_t>(m_buffer) + currentOffset, alignment);
            size_t alignedOffset = aligned - reinterpret_cast<uintptr_t>(m_buffer);
            size_t newOffset = alignedOffset + size;
            
            // Check if we have space
            if (newOffset > m_size) {
                return nullptr; // Out of memory
            }
            
            // Try to update offset atomically
            while (!m_offset.compare_exchange_weak(currentOffset, newOffset, 
                                                  std::memory_order_relaxed)) {
                // Recalculate with new current offset
                aligned = alignUp(reinterpret_cast<uintptr_t>(m_buffer) + currentOffset, alignment);
                alignedOffset = aligned - reinterpret_cast<uintptr_t>(m_buffer);
                newOffset = alignedOffset + size;
                
                if (newOffset > m_size) {
                    return nullptr;
                }
            }
            
            return reinterpret_cast<void*>(aligned);
        }

        // Linear allocator doesn't support individual deallocation
        void deallocate(void* ptr) override {
            // No-op - use reset() to free all memory
        }

        // Reset allocator (typically called each frame)
        void reset() {
            m_offset.store(0, std::memory_order_relaxed);
        }

        // Allocator info
        size_t getUsedMemory() const override {
            return m_offset.load(std::memory_order_relaxed);
        }

        size_t getTotalMemory() const override { return m_size; }
        const char* getName() const override { return m_name; }

        // Additional utilities
        template<typename T, typename... Args>
        T* construct(Args&&... args) {
            void* ptr = allocate(sizeof(T), alignof(T));
            return ptr ? new(ptr) T(std::forward<Args>(args)...) : nullptr;
        }

        template<typename T>
        T* allocateArray(size_t count) {
            return static_cast<T*>(allocate(sizeof(T) * count, alignof(T)));
        }

    private:
        char* m_buffer;
        size_t m_size;
        std::atomic<size_t> m_offset;
        const char* m_name;
    };

    // Stack allocator for LIFO allocations
    class StackAllocator : public IAllocator {
    public:
        explicit StackAllocator(size_t size, const char* name = "StackAllocator")
            : m_buffer(nullptr), m_size(size), m_top(0), m_name(name) {
#ifdef _WIN32
            m_buffer = static_cast<char*>(_aligned_malloc(size, CACHE_LINE_SIZE));
#else
            m_buffer = static_cast<char*>(std::aligned_alloc(CACHE_LINE_SIZE, size));
#endif
            assert(m_buffer && "Failed to allocate stack buffer");
        }

        ~StackAllocator() {
            if (m_buffer) {
#ifdef _WIN32
                _aligned_free(m_buffer);
#else
                std::free(m_buffer);
#endif
            }
        }

        // Allocation marker for scoped allocations
        struct Marker {
            size_t position;
            explicit Marker(size_t pos) : position(pos) {}
        };

        // Get current stack position
        Marker getMarker() const {
            std::lock_guard<std::mutex> lock(m_mutex);
            return Marker(m_top);
        }

        // Restore stack to marker position
        void restoreToMarker(const Marker& marker) {
            std::lock_guard<std::mutex> lock(m_mutex);
            assert(marker.position <= m_top);
            m_top = marker.position;
        }

        // Allocate from stack
        void* allocate(size_t size, size_t alignment = DEFAULT_ALIGNMENT) override {
            std::lock_guard<std::mutex> lock(m_mutex);
            
            uintptr_t aligned = alignUp(reinterpret_cast<uintptr_t>(m_buffer) + m_top, alignment);
            size_t alignedOffset = aligned - reinterpret_cast<uintptr_t>(m_buffer);
            size_t newTop = alignedOffset + size;
            
            if (newTop > m_size) {
                return nullptr;
            }
            
            m_top = newTop;
            return reinterpret_cast<void*>(aligned);
        }

        void deallocate(void* ptr) override {
            // Stack allocator uses markers for deallocation
        }

        size_t getUsedMemory() const override {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_top;
        }

        size_t getTotalMemory() const override { return m_size; }
        const char* getName() const override { return m_name; }

    private:
        char* m_buffer;
        size_t m_size;
        size_t m_top;
        const char* m_name;
        mutable std::mutex m_mutex;
    };

    // Memory manager for coordinating different allocators
    class MemoryManager {
    public:
        static MemoryManager& getInstance() {
            static MemoryManager instance;
            return instance;
        }

        // Initialize with memory budgets (in MB)
        void initialize(size_t frameMemoryMB = 16, size_t stackMemoryMB = 8) {
            m_frameAllocator = std::make_unique<LinearAllocator>(frameMemoryMB * 1024 * 1024, "FrameAllocator");
            m_stackAllocator = std::make_unique<StackAllocator>(stackMemoryMB * 1024 * 1024, "StackAllocator");
            
            m_allocators.push_back(m_frameAllocator.get());
            m_allocators.push_back(m_stackAllocator.get());
        }

        void shutdown() {
            m_allocators.clear();
            m_frameAllocator.reset();
            m_stackAllocator.reset();
        }

        // Per-frame reset
        void newFrame() {
            if (m_frameAllocator) {
                m_frameAllocator->reset();
            }
        }

        // Allocator access
        LinearAllocator* getFrameAllocator() { return m_frameAllocator.get(); }
        StackAllocator* getStackAllocator() { return m_stackAllocator.get(); }

        // Memory statistics
        void printMemoryStats() const;

        // RAII helper for stack allocations
        class StackScope {
        public:
            explicit StackScope(StackAllocator& allocator) 
                : m_allocator(allocator), m_marker(allocator.getMarker()) {}
            
            ~StackScope() {
                m_allocator.restoreToMarker(m_marker);
            }

        private:
            StackAllocator& m_allocator;
            StackAllocator::Marker m_marker;
        };

    private:
        MemoryManager() = default;
        ~MemoryManager() = default;

        std::unique_ptr<LinearAllocator> m_frameAllocator;
        std::unique_ptr<StackAllocator> m_stackAllocator;
        std::vector<IAllocator*> m_allocators;
    };

    // Convenience macros
    #define FRAME_ALLOC(type, ...) \
        ::AstralEngine::Memory::MemoryManager::getInstance().getFrameAllocator()->construct<type>(__VA_ARGS__)

    #define FRAME_ALLOC_ARRAY(type, count) \
        ::AstralEngine::Memory::MemoryManager::getInstance().getFrameAllocator()->template allocateArray<type>(count)

    #define STACK_SCOPE() \
        ::AstralEngine::Memory::MemoryManager::StackScope _stackScope(*::AstralEngine::Memory::MemoryManager::getInstance().getStackAllocator())

} // namespace AstralEngine::Memory
