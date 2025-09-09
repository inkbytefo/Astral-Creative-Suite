#pragma once

/**
 * @file Events.h
 * @brief Convenience header that includes all event system components
 * 
 * This header provides a single include point for the entire event system.
 * Include this file to get access to all event types, the event manager,
 * and related utilities.
 */

// Core event system
#include "Event.h"
#include "EventManager.h"

// Specific event types
#include "ApplicationEvent.h"
#include "InputEvent.h"

namespace AstralEngine {
    
    /**
     * @brief Initialize the event system
     * 
     * Call this during engine initialization to set up the event system.
     * This function is optional as the EventManager uses lazy initialization,
     * but calling it explicitly can help with debugging and ensures
     * the event system is ready to use.
     */
    inline void InitializeEventSystem() {
        auto& eventManager = EventManager::getInstance();
        eventManager.setLoggingEnabled(false); // Enable for debugging if needed
        AE_INFO("Event system initialized successfully");
    }
    
    /**
     * @brief Shutdown the event system
     * 
     * Call this during engine shutdown to clean up event subscriptions
     * and clear any queued events.
     */
    inline void ShutdownEventSystem() {
        auto& eventManager = EventManager::getInstance();
        eventManager.clearEventQueue();
        eventManager.unsubscribeAll();
        AE_INFO("Event system shutdown complete");
    }
    
} // namespace AstralEngine
