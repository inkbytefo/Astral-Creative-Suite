#include "MemoryManager.h"
#include "Logger.h"
#include <algorithm>
#include <cstring>
#include <sstream>

namespace AstralEngine {
    namespace Memory {
        void MemoryManager::initialize(size_t frameSizeMB, size_t stackSizeMB) {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_initialized) {
                AE_WARN("MemoryManager already initialized");
                return;
            }
            
            try {
                m_frameAllocator = std::make_unique<FrameAllocator>(frameSizeMB * 1024 * 1024);
                m_stackAllocator = std::make_unique<StackAllocator>(stackSizeMB * 1024 * 1024);
                m_initialized = true;
                
                std::stringstream ss;
                ss << "MemoryManager initialized with frameSize: " << frameSizeMB << " MB, stackSize: " << stackSizeMB << " MB";
                AE_INFO(ss.str());
            } catch (const std::exception& e) {
                std::stringstream ss;
                ss << "Failed to initialize MemoryManager: " << e.what();
                AE_ERROR(ss.str());
                m_initialized = false;
            }
        }
        
        void MemoryManager::shutdown() {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!m_initialized) {
                AE_WARN("MemoryManager not initialized");
                return;
            }
            
            m_frameAllocator.reset();
            m_stackAllocator.reset();
            m_initialized = false;
            AE_INFO("MemoryManager shutdown");
        }
        
        void MemoryManager::newFrame() {
            if (!m_initialized) {
                AE_WARN("MemoryManager not initialized");
                return;
            }
            
            if (m_frameAllocator) {
                m_frameAllocator->reset();
            }
        }
        
        MemoryManager::FrameAllocator* MemoryManager::getFrameAllocator() {
            if (!m_initialized) {
                AE_WARN("MemoryManager not initialized");
                return nullptr;
            }
            return m_frameAllocator.get();
        }
        
        MemoryManager::StackAllocator* MemoryManager::getStackAllocator() {
            if (!m_initialized) {
                AE_WARN("MemoryManager not initialized");
                return nullptr;
            }
            return m_stackAllocator.get();
        }
        
        // FrameAllocator implementation
        MemoryManager::FrameAllocator::FrameAllocator(size_t size) 
            : m_totalSize(size), m_usedSize(0) {
            m_memory = std::make_unique<uint8_t[]>(size);
            if (!m_memory) {
                throw std::bad_alloc();
            }
        }
        
        MemoryManager::FrameAllocator::~FrameAllocator() = default;
        
        void* MemoryManager::FrameAllocator::allocate(size_t size) {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_usedSize + size > m_totalSize) {
                AE_ERROR("FrameAllocator out of memory");
                return nullptr;
            }
            
            void* ptr = m_memory.get() + m_usedSize;
            m_usedSize += size;
            return ptr;
        }
        
        void MemoryManager::FrameAllocator::reset() {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_usedSize = 0;
        }
        
        size_t MemoryManager::FrameAllocator::getUsedMemory() const {
            return m_usedSize;
        }
        
        size_t MemoryManager::FrameAllocator::getTotalMemory() const {
            return m_totalSize;
        }
        
        // StackAllocator implementation
        MemoryManager::StackAllocator::StackAllocator(size_t size) 
            : m_totalSize(size), m_usedSize(0) {
            m_memory = std::make_unique<uint8_t[]>(size);
            if (!m_memory) {
                throw std::bad_alloc();
            }
        }
        
        MemoryManager::StackAllocator::~StackAllocator() = default;
        
        void* MemoryManager::StackAllocator::allocate(size_t size) {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_usedSize + size > m_totalSize) {
                AE_ERROR("StackAllocator out of memory");
                return nullptr;
            }
            
            void* ptr = m_memory.get() + m_usedSize;
            m_usedSize += size;
            return ptr;
        }
        
        void* MemoryManager::StackAllocator::allocateAligned(size_t size, size_t alignment) {
            std::lock_guard<std::mutex> lock(m_mutex);
            // Align the allocation
            size_t currentAddress = reinterpret_cast<size_t>(m_memory.get()) + m_usedSize;
            size_t alignedAddress = (currentAddress + alignment - 1) & ~(alignment - 1);
            size_t offset = alignedAddress - currentAddress;
            
            if (m_usedSize + size + offset > m_totalSize) {
                AE_ERROR("StackAllocator out of memory");
                return nullptr;
            }
            
            m_usedSize += offset;
            void* ptr = m_memory.get() + m_usedSize;
            m_usedSize += size;
            return ptr;
        }
        
        void MemoryManager::StackAllocator::reset() {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_usedSize = 0;
        }
        
        size_t MemoryManager::StackAllocator::getUsedMemory() const {
            return m_usedSize;
        }
        
        size_t MemoryManager::StackAllocator::getTotalMemory() const {
            return m_totalSize;
        }
    }
}
