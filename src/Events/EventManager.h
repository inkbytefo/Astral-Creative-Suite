#pragma once

#include "Event.h"
#include "Core/Logger.h"
#include <vector>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <memory>
#include <atomic>
#include <algorithm>

namespace AstralEngine {

    /**
     * @brief Event subscription information
     */
    struct EventSubscriptionInfo {
        uint64_t id;
        EventType eventType;
        EventHandler handler;
        EventPriority priority;
        bool active;

        EventSubscriptionInfo(uint64_t subscriptionId, EventType type, 
                            const EventHandler& h, EventPriority p)
            : id(subscriptionId), eventType(type), handler(h), priority(p), active(true) {}
    };

    /**
     * @brief Event queue entry for deferred event processing
     */
    struct QueuedEvent {
        std::unique_ptr<Event> event;
        EventPriority priority;

        QueuedEvent(std::unique_ptr<Event> e, EventPriority p)
            : event(std::move(e)), priority(p) {}

        // For priority queue (higher priority first)
        bool operator<(const QueuedEvent& other) const {
            return priority < other.priority;
        }
    };

    /**
     * @brief Global Event Manager (Singleton)
     * 
     * The EventManager is the central hub for all event communication in the engine.
     * It provides both immediate and deferred event processing, subscription management,
     * and thread-safe event handling.
     * 
     * Features:
     * - Global event bus with type-safe subscriptions
     * - Priority-based event processing
     * - Immediate and deferred event dispatch
     * - Thread-safe operations
     * - RAII subscription management
     * - Event filtering and statistics
     * - Memory-efficient event storage
     */
    class EventManager {
    public:
        static EventManager& getInstance();
        
        // Non-copyable and non-movable
        EventManager(const EventManager&) = delete;
        EventManager& operator=(const EventManager&) = delete;
        EventManager(EventManager&&) = delete;
        EventManager& operator=(EventManager&&) = delete;

        /**
         * @brief Subscribe to a specific event type
         * @param eventType The type of event to subscribe to
         * @param handler The handler function to call when the event occurs
         * @param priority Priority level for this subscription
         * @return RAII subscription handle
         */
        EventSubscription subscribe(EventType eventType, const EventHandler& handler, 
                                  EventPriority priority = EventPriority::Normal);

        /**
         * @brief Subscribe to multiple event types with the same handler
         */
        std::vector<EventSubscription> subscribe(const std::vector<EventType>& eventTypes, 
                                               const EventHandler& handler,
                                               EventPriority priority = EventPriority::Normal);

        /**
         * @brief Subscribe using an event listener interface
         */
        EventSubscription subscribe(EventType eventType, IEventListener* listener,
                                  EventPriority priority = EventPriority::Normal);

        /**
         * @brief Dispatch event immediately to all subscribers
         * @param event The event to dispatch
         * @return true if event was handled by any subscriber
         */
        bool dispatchEvent(Event& event);

        /**
         * @brief Queue event for deferred processing
         * @param event The event to queue (takes ownership)
         */
        void queueEvent(std::unique_ptr<Event> event);

        /**
         * @brief Process all queued events
         * @param maxEvents Maximum number of events to process (0 = all)
         * @return Number of events processed
         */
        uint32_t processQueuedEvents(uint32_t maxEvents = 0);

        /**
         * @brief Clear all queued events
         */
        void clearEventQueue();

        /**
         * @brief Remove a subscription
         */
        void unsubscribe(uint64_t subscriptionId);

        /**
         * @brief Remove all subscriptions for a specific event type
         */
        void unsubscribeAll(EventType eventType);

        /**
         * @brief Remove all subscriptions
         */
        void unsubscribeAll();

        /**
         * @brief Get number of subscribers for an event type
         */
        size_t getSubscriberCount(EventType eventType) const;

        /**
         * @brief Get total number of active subscriptions
         */
        size_t getTotalSubscriberCount() const;

        /**
         * @brief Get number of queued events
         */
        size_t getQueuedEventCount() const;

        /**
         * @brief Enable/disable event processing
         */
        void setEnabled(bool enabled) { m_enabled = enabled; }
        bool isEnabled() const { return m_enabled; }

        /**
         * @brief Enable/disable event logging for debugging
         */
        void setLoggingEnabled(bool enabled) { m_loggingEnabled = enabled; }
        bool isLoggingEnabled() const { return m_loggingEnabled; }

        /**
         * @brief Get event statistics
         */
        struct EventStats {
            uint64_t totalEventsDispatched = 0;
            uint64_t totalEventsQueued = 0;
            uint64_t totalEventsProcessed = 0;
            uint64_t totalSubscriptions = 0;
            uint64_t activeSubscriptions = 0;
        };
        EventStats getStats() const;

        /**
         * @brief Reset statistics
         */
        void resetStats();

        /**
         * @brief Log current event manager state for debugging
         */
        void logDebugInfo() const;

    private:
        EventManager() = default;
        ~EventManager() = default;

        // Subscription management
        std::unordered_map<EventType, std::vector<EventSubscriptionInfo>> m_subscribers;
        mutable std::mutex m_subscribersMutex;
        std::atomic<uint64_t> m_nextSubscriptionId{1};

        // Event queue
        std::priority_queue<QueuedEvent> m_eventQueue;
        mutable std::mutex m_queueMutex;

        // State
        std::atomic<bool> m_enabled{true};
        std::atomic<bool> m_loggingEnabled{false};

        // Statistics
        mutable std::mutex m_statsMutex;
        EventStats m_stats;

        // Helper methods
        void sortSubscribersByPriority(std::vector<EventSubscriptionInfo>& subscribers);
        void cleanupInactiveSubscriptions(EventType eventType);
        void logEvent(const Event& event, const char* action) const;
    };

    //=============================================================================
    // EventSubscription Implementation  
    //=============================================================================

    inline EventSubscription::EventSubscription(EventManager* manager, uint64_t id)
        : m_manager(manager), m_subscriptionId(id) {}

    inline EventSubscription::~EventSubscription() {
        unsubscribe();
    }

    inline EventSubscription::EventSubscription(EventSubscription&& other) noexcept
        : m_manager(other.m_manager), m_subscriptionId(other.m_subscriptionId) {
        other.m_manager = nullptr;
        other.m_subscriptionId = 0;
    }

    inline EventSubscription& EventSubscription::operator=(EventSubscription&& other) noexcept {
        if (this != &other) {
            unsubscribe();
            m_manager = other.m_manager;
            m_subscriptionId = other.m_subscriptionId;
            other.m_manager = nullptr;
            other.m_subscriptionId = 0;
        }
        return *this;
    }

    inline void EventSubscription::unsubscribe() {
        if (m_manager && m_subscriptionId != 0) {
            m_manager->unsubscribe(m_subscriptionId);
            m_manager = nullptr;
            m_subscriptionId = 0;
        }
    }

    //=============================================================================
    // Convenience Macros and Functions
    //=============================================================================

    /**
     * @brief Global event dispatch function
     */
    inline bool DispatchEvent(Event& event) {
        return EventManager::getInstance().dispatchEvent(event);
    }

    /**
     * @brief Global event queue function
     */
    template<typename T, typename... Args>
    inline void QueueEvent(Args&&... args) {
        auto event = std::make_unique<T>(std::forward<Args>(args)...);
        EventManager::getInstance().queueEvent(std::move(event));
    }

    /**
     * @brief Global subscription function
     */
    inline EventSubscription Subscribe(EventType eventType, const EventHandler& handler, 
                                     EventPriority priority = EventPriority::Normal) {
        return EventManager::getInstance().subscribe(eventType, handler, priority);
    }

} // namespace AstralEngine
