#include "EventManager.h"
#include <algorithm>

namespace AstralEngine {

    EventManager& EventManager::getInstance() {
        static EventManager instance;
        return instance;
    }

    EventSubscription EventManager::subscribe(EventType eventType, const EventHandler& handler, EventPriority priority) {
        if (!handler) {
            AE_WARN("Attempting to subscribe with null handler for event type {}", static_cast<uint32_t>(eventType));
            return EventSubscription();
        }

        std::lock_guard<std::mutex> lock(m_subscribersMutex);
        
        uint64_t subscriptionId = m_nextSubscriptionId++;
        m_subscribers[eventType].emplace_back(subscriptionId, eventType, handler, priority);
        
        // Sort by priority (higher priority first)
        sortSubscribersByPriority(m_subscribers[eventType]);
        
        {
            std::lock_guard<std::mutex> statsLock(m_statsMutex);
            m_stats.totalSubscriptions++;
            m_stats.activeSubscriptions++;
        }
        
        if (m_loggingEnabled) {
            AE_DEBUG("Event subscription created: ID={}, Type={}, Priority={}", 
                    subscriptionId, static_cast<uint32_t>(eventType), static_cast<uint8_t>(priority));
        }
        
        return EventSubscription(this, subscriptionId);
    }

    std::vector<EventSubscription> EventManager::subscribe(const std::vector<EventType>& eventTypes, 
                                                         const EventHandler& handler, EventPriority priority) {
        std::vector<EventSubscription> subscriptions;
        subscriptions.reserve(eventTypes.size());
        
        for (EventType eventType : eventTypes) {
            subscriptions.push_back(subscribe(eventType, handler, priority));
        }
        
        return subscriptions;
    }

    EventSubscription EventManager::subscribe(EventType eventType, IEventListener* listener, EventPriority priority) {
        if (!listener) {
            AE_WARN("Attempting to subscribe with null listener for event type {}", static_cast<uint32_t>(eventType));
            return EventSubscription();
        }
        
        EventHandler handler = [listener](Event& event) -> bool {
            return listener->onEvent(event);
        };
        
        return subscribe(eventType, handler, priority);
    }

    bool EventManager::dispatchEvent(Event& event) {
        if (!m_enabled) {
            return false;
        }
        
        if (m_loggingEnabled) {
            logEvent(event, "Dispatching");
        }
        
        bool handled = false;
        EventType eventType = event.getEventType();
        
        {
            std::lock_guard<std::mutex> lock(m_subscribersMutex);
            auto it = m_subscribers.find(eventType);
            if (it != m_subscribers.end()) {
                auto& subscribers = it->second;
                
                // Process subscribers in priority order (already sorted)
                for (auto& subscription : subscribers) {
                    if (!subscription.active) continue;
                    
                    try {
                        bool result = subscription.handler(event);
                        handled = handled || result;
                        
                        // Stop processing if event was handled and marked as such
                        if (event.handled) {
                            if (m_loggingEnabled) {
                                AE_DEBUG("Event handled by subscription ID={}, stopping propagation", subscription.id);
                            }
                            break;
                        }
                    } catch (const std::exception& e) {
                        AE_ERROR("Exception in event handler (subscription ID={}): {}", subscription.id, e.what());
                    } catch (...) {
                        AE_ERROR("Unknown exception in event handler (subscription ID={})", subscription.id);
                    }
                }
            }
        }
        
        {
            std::lock_guard<std::mutex> statsLock(m_statsMutex);
            m_stats.totalEventsDispatched++;
        }
        
        if (m_loggingEnabled) {
            AE_DEBUG("Event dispatch complete: handled={}", handled);
        }
        
        return handled;
    }

    void EventManager::queueEvent(std::unique_ptr<Event> event) {
        if (!event) {
            AE_WARN("Attempting to queue null event");
            return;
        }
        
        if (!m_enabled) {
            return;
        }
        
        EventPriority priority = event->getPriority();
        
        if (m_loggingEnabled) {
            logEvent(*event, "Queuing");
        }
        
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            m_eventQueue.emplace(std::move(event), priority);
        }
        
        {
            std::lock_guard<std::mutex> statsLock(m_statsMutex);
            m_stats.totalEventsQueued++;
        }
    }

    uint32_t EventManager::processQueuedEvents(uint32_t maxEvents) {
        if (!m_enabled) {
            return 0;
        }
        
        uint32_t processedCount = 0;
        
        while ((maxEvents == 0 || processedCount < maxEvents)) {
            std::unique_ptr<Event> event;
            
            {
                std::lock_guard<std::mutex> lock(m_queueMutex);
                if (m_eventQueue.empty()) {
                    break;
                }
                
                event = std::move(const_cast<QueuedEvent&>(m_eventQueue.top()).event);
                m_eventQueue.pop();
            }
            
            if (event) {
                dispatchEvent(*event);
                processedCount++;
                
                {
                    std::lock_guard<std::mutex> statsLock(m_statsMutex);
                    m_stats.totalEventsProcessed++;
                }
            }
        }
        
        return processedCount;
    }

    void EventManager::clearEventQueue() {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        
        // Clear the priority queue by creating a new empty one
        std::priority_queue<QueuedEvent> emptyQueue;
        m_eventQueue.swap(emptyQueue);
        
        AE_DEBUG("Event queue cleared");
    }

    void EventManager::unsubscribe(uint64_t subscriptionId) {
        if (subscriptionId == 0) return;
        
        std::lock_guard<std::mutex> lock(m_subscribersMutex);
        
        for (auto& [eventType, subscribers] : m_subscribers) {
            auto it = std::find_if(subscribers.begin(), subscribers.end(),
                [subscriptionId](const EventSubscriptionInfo& info) {
                    return info.id == subscriptionId;
                });
            
            if (it != subscribers.end()) {
                it->active = false; // Mark as inactive instead of removing immediately
                
                {
                    std::lock_guard<std::mutex> statsLock(m_statsMutex);
                    m_stats.activeSubscriptions--;
                }
                
                if (m_loggingEnabled) {
                    AE_DEBUG("Event subscription marked inactive: ID={}, Type={}", 
                            subscriptionId, static_cast<uint32_t>(eventType));
                }
                
                // Schedule cleanup
                cleanupInactiveSubscriptions(eventType);
                return;
            }
        }
    }

    void EventManager::unsubscribeAll(EventType eventType) {
        std::lock_guard<std::mutex> lock(m_subscribersMutex);
        
        auto it = m_subscribers.find(eventType);
        if (it != m_subscribers.end()) {
            size_t count = it->second.size();
            m_subscribers.erase(it);
            
            {
                std::lock_guard<std::mutex> statsLock(m_statsMutex);
                m_stats.activeSubscriptions -= count;
            }
            
            AE_DEBUG("Removed all {} subscriptions for event type {}", count, static_cast<uint32_t>(eventType));
        }
    }

    void EventManager::unsubscribeAll() {
        std::lock_guard<std::mutex> lock(m_subscribersMutex);
        
        size_t totalCount = 0;
        for (const auto& [eventType, subscribers] : m_subscribers) {
            totalCount += subscribers.size();
        }
        
        m_subscribers.clear();
        
        {
            std::lock_guard<std::mutex> statsLock(m_statsMutex);
            m_stats.activeSubscriptions = 0;
        }
        
        AE_DEBUG("Removed all {} event subscriptions", totalCount);
    }

    size_t EventManager::getSubscriberCount(EventType eventType) const {
        std::lock_guard<std::mutex> lock(m_subscribersMutex);
        
        auto it = m_subscribers.find(eventType);
        if (it != m_subscribers.end()) {
            return std::count_if(it->second.begin(), it->second.end(),
                [](const EventSubscriptionInfo& info) { return info.active; });
        }
        return 0;
    }

    size_t EventManager::getTotalSubscriberCount() const {
        std::lock_guard<std::mutex> statsLock(m_statsMutex);
        return m_stats.activeSubscriptions;
    }

    size_t EventManager::getQueuedEventCount() const {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        return m_eventQueue.size();
    }

    EventManager::EventStats EventManager::getStats() const {
        std::lock_guard<std::mutex> lock(m_statsMutex);
        return m_stats;
    }

    void EventManager::resetStats() {
        std::lock_guard<std::mutex> lock(m_statsMutex);
        m_stats = EventStats{};
        m_stats.activeSubscriptions = getTotalSubscriberCount();
        AE_DEBUG("Event manager statistics reset");
    }

    void EventManager::logDebugInfo() const {
        AE_INFO("=== Event Manager Debug Info ===");
        AE_INFO("Enabled: {}", m_enabled.load());
        AE_INFO("Logging Enabled: {}", m_loggingEnabled.load());
        AE_INFO("Queued Events: {}", getQueuedEventCount());
        AE_INFO("Total Subscribers: {}", getTotalSubscriberCount());
        
        {
            std::lock_guard<std::mutex> lock(m_subscribersMutex);
            AE_INFO("Event Types Subscribed: {}", m_subscribers.size());
            
            for (const auto& [eventType, subscribers] : m_subscribers) {
                size_t activeCount = std::count_if(subscribers.begin(), subscribers.end(),
                    [](const EventSubscriptionInfo& info) { return info.active; });
                AE_INFO("  Type {}: {} active subscribers", static_cast<uint32_t>(eventType), activeCount);
            }
        }
        
        auto stats = getStats();
        AE_INFO("Statistics:");
        AE_INFO("  Total Events Dispatched: {}", stats.totalEventsDispatched);
        AE_INFO("  Total Events Queued: {}", stats.totalEventsQueued);
        AE_INFO("  Total Events Processed: {}", stats.totalEventsProcessed);
        AE_INFO("  Total Subscriptions Created: {}", stats.totalSubscriptions);
        AE_INFO("  Active Subscriptions: {}", stats.activeSubscriptions);
        AE_INFO("================================");
    }

    void EventManager::sortSubscribersByPriority(std::vector<EventSubscriptionInfo>& subscribers) {
        std::sort(subscribers.begin(), subscribers.end(),
            [](const EventSubscriptionInfo& a, const EventSubscriptionInfo& b) {
                return a.priority > b.priority; // Higher priority first
            });
    }

    void EventManager::cleanupInactiveSubscriptions(EventType eventType) {
        // Remove inactive subscriptions (called periodically to prevent memory leaks)
        auto it = m_subscribers.find(eventType);
        if (it != m_subscribers.end()) {
            auto& subscribers = it->second;
            size_t beforeSize = subscribers.size();
            
            subscribers.erase(std::remove_if(subscribers.begin(), subscribers.end(),
                [](const EventSubscriptionInfo& info) { return !info.active; }),
                subscribers.end());
            
            size_t removedCount = beforeSize - subscribers.size();
            if (removedCount > 0 && m_loggingEnabled) {
                AE_DEBUG("Cleaned up {} inactive subscriptions for event type {}", 
                        removedCount, static_cast<uint32_t>(eventType));
            }
        }
    }

    void EventManager::logEvent(const Event& event, const char* action) const {
        if (m_loggingEnabled) {
            AE_DEBUG("{} event: {} [{}]", action, event.getName(), event.toString());
        }
    }

} // namespace AstralEngine
