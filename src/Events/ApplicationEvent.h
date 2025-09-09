#pragma once

#include "Event.h"
#include <sstream>

namespace AstralEngine {

    //=============================================================================
    // Window Events
    //=============================================================================

    /**
     * @brief Window resize event
     * 
     * Fired when the window size changes. Contains the new dimensions.
     */
    class WindowResizeEvent : public Event {
    public:
        WindowResizeEvent(uint32_t width, uint32_t height)
            : m_width(width), m_height(height) {}

        uint32_t getWidth() const { return m_width; }
        uint32_t getHeight() const { return m_height; }

        std::string toString() const override {
            std::stringstream ss;
            ss << "WindowResizeEvent: " << m_width << "x" << m_height;
            return ss.str();
        }

        EVENT_CLASS_TYPE(WindowResize)
        EVENT_CLASS_CATEGORY(EventCategoryApplication)
        EVENT_CLASS_PRIORITY(High)

    private:
        uint32_t m_width, m_height;
    };

    /**
     * @brief Window close event
     * 
     * Fired when the user requests to close the window.
     */
    class WindowCloseEvent : public Event {
    public:
        WindowCloseEvent() = default;

        std::string toString() const override {
            return "WindowCloseEvent";
        }

        EVENT_CLASS_TYPE(WindowClose)
        EVENT_CLASS_CATEGORY(EventCategoryApplication)
        EVENT_CLASS_PRIORITY(Critical)
    };

    /**
     * @brief Window focus event
     * 
     * Fired when the window gains or loses focus.
     */
    class WindowFocusEvent : public Event {
    public:
        WindowFocusEvent(bool focused) : m_focused(focused) {}

        bool isFocused() const { return m_focused; }

        std::string toString() const override {
            std::stringstream ss;
            ss << "WindowFocusEvent: " << (m_focused ? "gained" : "lost");
            return ss.str();
        }

        EVENT_CLASS_TYPE(WindowFocus)
        EVENT_CLASS_CATEGORY(EventCategoryApplication)

    private:
        bool m_focused;
    };

    /**
     * @brief Window lost focus event
     * 
     * Fired when the window loses focus.
     */
    class WindowLostFocusEvent : public Event {
    public:
        WindowLostFocusEvent() = default;

        std::string toString() const override {
            return "WindowLostFocusEvent";
        }

        EVENT_CLASS_TYPE(WindowLostFocus)
        EVENT_CLASS_CATEGORY(EventCategoryApplication)
    };

    /**
     * @brief Window moved event
     * 
     * Fired when the window position changes.
     */
    class WindowMovedEvent : public Event {
    public:
        WindowMovedEvent(int32_t x, int32_t y) : m_x(x), m_y(y) {}

        int32_t getX() const { return m_x; }
        int32_t getY() const { return m_y; }

        std::string toString() const override {
            std::stringstream ss;
            ss << "WindowMovedEvent: (" << m_x << ", " << m_y << ")";
            return ss.str();
        }

        EVENT_CLASS_TYPE(WindowMoved)
        EVENT_CLASS_CATEGORY(EventCategoryApplication)

    private:
        int32_t m_x, m_y;
    };

    /**
     * @brief Window minimized event
     */
    class WindowMinimizedEvent : public Event {
    public:
        WindowMinimizedEvent() = default;

        std::string toString() const override {
            return "WindowMinimizedEvent";
        }

        EVENT_CLASS_TYPE(WindowMinimized)
        EVENT_CLASS_CATEGORY(EventCategoryApplication)
    };

    /**
     * @brief Window maximized event
     */
    class WindowMaximizedEvent : public Event {
    public:
        WindowMaximizedEvent() = default;

        std::string toString() const override {
            return "WindowMaximizedEvent";
        }

        EVENT_CLASS_TYPE(WindowMaximized)
        EVENT_CLASS_CATEGORY(EventCategoryApplication)
    };

    /**
     * @brief Window restored event
     */
    class WindowRestoredEvent : public Event {
    public:
        WindowRestoredEvent() = default;

        std::string toString() const override {
            return "WindowRestoredEvent";
        }

        EVENT_CLASS_TYPE(WindowRestored)
        EVENT_CLASS_CATEGORY(EventCategoryApplication)
    };

    //=============================================================================
    // Application Lifecycle Events
    //=============================================================================

    /**
     * @brief Application tick event
     * 
     * Fired every frame for timing-critical updates.
     */
    class AppTickEvent : public Event {
    public:
        AppTickEvent(float deltaTime) : m_deltaTime(deltaTime) {}

        float getDeltaTime() const { return m_deltaTime; }

        std::string toString() const override {
            std::stringstream ss;
            ss << "AppTickEvent: dt=" << m_deltaTime;
            return ss.str();
        }

        EVENT_CLASS_TYPE(AppTick)
        EVENT_CLASS_CATEGORY(EventCategoryApplication)
        EVENT_CLASS_PRIORITY(High)

    private:
        float m_deltaTime;
    };

    /**
     * @brief Application update event
     * 
     * Fired every frame for general updates.
     */
    class AppUpdateEvent : public Event {
    public:
        AppUpdateEvent(float deltaTime) : m_deltaTime(deltaTime) {}

        float getDeltaTime() const { return m_deltaTime; }

        std::string toString() const override {
            std::stringstream ss;
            ss << "AppUpdateEvent: dt=" << m_deltaTime;
            return ss.str();
        }

        EVENT_CLASS_TYPE(AppUpdate)
        EVENT_CLASS_CATEGORY(EventCategoryApplication)

    private:
        float m_deltaTime;
    };

    /**
     * @brief Application render event
     * 
     * Fired when the application should render.
     */
    class AppRenderEvent : public Event {
    public:
        AppRenderEvent() = default;

        std::string toString() const override {
            return "AppRenderEvent";
        }

        EVENT_CLASS_TYPE(AppRender)
        EVENT_CLASS_CATEGORY(EventCategoryApplication)
        EVENT_CLASS_PRIORITY(Normal)
    };

} // namespace AstralEngine
