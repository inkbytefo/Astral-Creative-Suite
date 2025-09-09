#pragma once

#include "Core/Logger.h"
#include <string>
#include <functional>
#include <vector>
#include <unordered_map>
#include <memory>
#include <typeindex>
#include <chrono>
#include <cstdint>

namespace AstralEngine {

    // Forward declarations
    class Event;
    class EventDispatcher;
    class EventManager;
    
    /**
     * @brief Event types enumeration
     * 
     * Comprehensive list of all event types in the engine.
     * Each event type has a unique identifier for fast dispatching.
     */
    enum class EventType : uint32_t {
        None = 0,
        
        // Application Events (1-99)
        WindowClose = 1,
        WindowResize = 2, 
        WindowFocus = 3,
        WindowLostFocus = 4,
        WindowMoved = 5,
        WindowMinimized = 6,
        WindowMaximized = 7,
        WindowRestored = 8,
        AppTick = 9,
        AppUpdate = 10,
        AppRender = 11,
        
        // Input Events (100-199)
        KeyPressed = 100,
        KeyReleased = 101,
        KeyTyped = 102,
        
        // Mouse Events (200-299)
        MouseButtonPressed = 200,
        MouseButtonReleased = 201,
        MouseMoved = 202,
        MouseScrolled = 203,
        MouseEntered = 204,
        MouseLeft = 205,
        
        // Engine Events (300-399)
        SceneChanged = 300,
        AssetLoaded = 301,
        AssetUnloaded = 302,
        ShaderRecompiled = 303,
        
        // Custom/User Events (1000+)
        Custom = 1000
    };
    
    /**
     * @brief Event categories (bitfield)
     * 
     * Categories allow filtering and subscribing to groups of events.
     * Multiple categories can be combined using bitwise OR.
     */
    enum EventCategory : uint32_t {
        EventCategoryNone = 0,
        EventCategoryApplication = (1 << 0),
        EventCategoryInput = (1 << 1),
        EventCategoryKeyboard = (1 << 2),
        EventCategoryMouse = (1 << 3),
        EventCategoryEngine = (1 << 4),
        EventCategoryCustom = (1 << 5)
    };
    
    /**
     * @brief Event priority levels
     * 
     * Higher priority events are processed first.
     */
    enum class EventPriority : uint8_t {
        Low = 0,
        Normal = 1,
        High = 2,
        Critical = 3
    };
    
    // Macros for event class boilerplate
    #define EVENT_CLASS_TYPE(type) \
        static EventType getStaticType() { return EventType::type; } \
        virtual EventType getEventType() const override { return getStaticType(); } \
        virtual const char* getName() const override { return #type; }

    #define EVENT_CLASS_CATEGORY(category) \
        virtual int getCategoryFlags() const override { return category; }
        
    #define EVENT_CLASS_PRIORITY(priority) \
        virtual EventPriority getPriority() const override { return EventPriority::priority; }
    
    /**
     * @brief Base Event class
     * 
     * All events in the AstralEngine inherit from this base class.
     * Provides type information, categorization, and basic event handling.
     * 
     * Features:
     * - Type-safe event identification
     * - Category-based filtering
     * - Priority system for event processing order
     * - Timestamp for event timing
     * - Handled flag to prevent further propagation
     * - String representation for debugging
     */
    class Event {
    public:
        Event() : m_timestamp(std::chrono::steady_clock::now()), handled(false) {}
        virtual ~Event() = default;
        
        // Event identification
        virtual EventType getEventType() const = 0;
        virtual const char* getName() const = 0;
        virtual int getCategoryFlags() const = 0;
        virtual EventPriority getPriority() const { return EventPriority::Normal; }
        
        // Event information
        virtual std::string toString() const { return getName(); }
        std::chrono::steady_clock::time_point getTimestamp() const { return m_timestamp; }
        
        // Category checking
        bool isInCategory(EventCategory category) const {
            return getCategoryFlags() & category;
        }
        
        // Event control
        bool handled = false;
        
    private:
        std::chrono::steady_clock::time_point m_timestamp;
    };
    
    /**
     * @brief Event handler function type
     */
    using EventHandler = std::function<bool(Event&)>;
    
    /**
     * @brief Event dispatcher for type-safe event handling
     * 
     * The EventDispatcher provides a convenient way to handle events
     * in a type-safe manner using template functions.
     */
    class EventDispatcher {
    public:
        EventDispatcher(Event& event) : m_event(event) {}
        
        /**
         * @brief Dispatch event to handler if types match
         * @param func Handler function that takes the specific event type
         * @return true if event was dispatched, false otherwise
         */
        template<typename T, typename F>
        bool dispatch(const F& func) {
            if (m_event.getEventType() == T::getStaticType()) {
                m_event.handled |= func(static_cast<T&>(m_event));
                return true;
            }
            return false;
        }
        
    private:
        Event& m_event;
    };
    
    /**
     * @brief Event listener interface
     * 
     * Classes that want to receive events should inherit from this interface.
     */
    class IEventListener {
    public:
        virtual ~IEventListener() = default;
        virtual bool onEvent(Event& event) = 0;
    };
    
    /**
     * @brief Event subscription handle
     * 
     * RAII wrapper for event subscriptions. Automatically unsubscribes
     * when destroyed to prevent dangling pointers.
     */
    class EventSubscription {
    public:
        EventSubscription() = default;
        EventSubscription(EventManager* manager, uint64_t id);
        ~EventSubscription();
        
        // Non-copyable, but movable
        EventSubscription(const EventSubscription&) = delete;
        EventSubscription& operator=(const EventSubscription&) = delete;
        EventSubscription(EventSubscription&& other) noexcept;
        EventSubscription& operator=(EventSubscription&& other) noexcept;
        
        void unsubscribe();
        bool isValid() const { return m_manager != nullptr; }
        
    private:
        EventManager* m_manager = nullptr;
        uint64_t m_subscriptionId = 0;
    };
    
} // namespace AstralEngine
