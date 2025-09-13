#include "EventManager.h"
#include <algorithm>
#include <stdexcept>
#include <sstream>

namespace AstralEngine {

    EventManager& EventManager::getInstance() {
        static EventManager instance;
        return instance;
    }

    EventSubscription EventManager::subscribe(EventType eventType, const EventHandler& handler, EventPriority priority) {
        if (!handler) {
            std::stringstream ss;
            ss << "Attempting to subscribe with null handler for event type " << static_cast<uint32_t>(eventType);
            AE_WARN(ss.str());
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
            std::stringstream ss;
            ss << "Event subscription created: ID=" << subscriptionId << ", Type=" << static_cast<uint32_t>(eventType) << ", Priority=" << static_cast<uint8_t>(priority);
            AE_DEBUG(ss.str());
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
            std::stringstream ss;
            ss << "Attempting to subscribe with null listener for event type " << static_cast<uint32_t>(eventType);
            AE_WARN(ss.str());
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
                                std::stringstream ss;
                                ss << "Event handled by subscription ID=" << subscription.id << ", stopping propagation";
                                AE_DEBUG(ss.str());
                            }
                            break;
                        }
                    } catch (const std::exception& e) {
                        std::stringstream ss;
                        ss << "Exception in event handler (subscription ID=" << subscription.id << "): " << e.what();
                        AE_ERROR(ss.str());
                    } catch (...) {
                        std::stringstream ss;
                        ss << "Unknown exception in event handler (subscription ID=" << subscription.id << ")";
                        AE_ERROR(ss.str());
                    }
                }
            }
        }
        
        {
            std::lock_guard<std::mutex> statsLock(m_statsMutex);
            m_stats.totalEventsDispatched++;
        }
        
        if (m_loggingEnabled) {
            std::stringstream ss;
            ss << "Event dispatch complete: handled=" << handled;
            AE_DEBUG(ss.str());
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
                    std::stringstream ss;
                    ss << "Event subscription marked inactive: ID=" << subscriptionId << ", Type=" << static_cast<uint32_t>(eventType);
                    AE_DEBUG(ss.str());
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
            
            std::stringstream ss;
            ss << "Removed all " << count << " subscriptions for event type " << static_cast<uint32_t>(eventType);
            AE_DEBUG(ss.str());
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
        
        std::stringstream ss;
        ss << "Removed all " << totalCount << " event subscriptions";
        AE_DEBUG(ss.str());
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
        std::stringstream ss1;
        ss1 << "Enabled: " << m_enabled.load();
        AE_INFO(ss1.str());
        std::stringstream ss2;
        ss2 << "Logging Enabled: " << m_loggingEnabled.load();
        AE_INFO(ss2.str());
        std::stringstream ss3;
        ss3 << "Queued Events: " << getQueuedEventCount();
        AE_INFO(ss3.str());
        std::stringstream ss4;
        ss4 << "Total Subscribers: " << getTotalSubscriberCount();
        AE_INFO(ss4.str());
        
        {
            std::lock_guard<std::mutex> lock(m_subscribersMutex);
            std::stringstream ss5;
            ss5 << "Event Types Subscribed: " << m_subscribers.size();
            AE_INFO(ss5.str());
            
            for (const auto& [eventType, subscribers] : m_subscribers) {
                size_t activeCount = std::count_if(subscribers.begin(), subscribers.end(),
                    [](const EventSubscriptionInfo& info) { return info.active; });
                std::stringstream ss6;
                ss6 << "  Type " << static_cast<uint32_t>(eventType) << ": " << activeCount << " active subscribers";
                AE_INFO(ss6.str());
            }
        }
        
        auto stats = getStats();
        AE_INFO("Statistics:");
        std::stringstream ss7;
        ss7 << "  Total Events Dispatched: " << stats.totalEventsDispatched;
        AE_INFO(ss7.str());
        std::stringstream ss8;
        ss8 << "  Total Events Queued: " << stats.totalEventsQueued;
        AE_INFO(ss8.str());
        std::stringstream ss9;
        ss9 << "  Total Events Processed: " << stats.totalEventsProcessed;
        AE_INFO(ss9.str());
        std::stringstream ss10;
        ss10 << "  Total Subscriptions Created: " << stats.totalSubscriptions;
        AE_INFO(ss10.str());
        std::stringstream ss11;
        ss11 << " Active Subscriptions: " << stats.activeSubscriptions;
        AE_INFO(ss11.str());
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
                std::stringstream ss;
                ss << "Cleaned up " << removedCount << " inactive subscriptions for event type " << static_cast<uint32_t>(eventType);
                AE_DEBUG(ss.str());
            }
        }
    }

    void EventManager::logEvent(const Event& event, const char* action) const {
        if (m_loggingEnabled) {
            std::stringstream ss;
            ss << action << " event: " << event.getName() << " [" << event.toString() << "]";
            AE_DEBUG(ss.str());
        }
    }

} // namespace AstralEngine
