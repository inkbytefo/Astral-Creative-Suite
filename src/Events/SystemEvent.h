#ifndef ASTRAL_ENGINE_SYSTEM_EVENT_H
#define ASTRAL_ENGINE_SYSTEM_EVENT_H

#include "Event.h"
#include <sstream>

namespace AstralEngine {

    //=============================================================================
    // System Events
    //=============================================================================

    /**
     * @brief Battery level change event
     * 
     * Fired when the system battery level changes significantly.
     */
    class BatteryLevelChangeEvent : public Event {
    public:
        BatteryLevelChangeEvent(float level, bool isCharging)
            : m_level(level), m_isCharging(isCharging) {}

        float getBatteryLevel() const { return m_level; }
        bool isCharging() const { return m_isCharging; }

        std::string toString() const override {
            std::stringstream ss;
            ss << "BatteryLevelChangeEvent: " << m_level * 100 << "% " 
               << (m_isCharging ? "(charging)" : "(discharging)");
            return ss.str();
        }

        EVENT_CLASS_TYPE(BatteryLevelChange)
        EVENT_CLASS_CATEGORY(EventCategoryEngine)
        EVENT_CLASS_PRIORITY(Normal)

    private:
        float m_level;      // 0.0 to 1.0
        bool m_isCharging;
    };

    /**
     * @brief Network connection change event
     * 
     * Fired when the network connection status changes.
     */
    class NetworkConnectionEvent : public Event {
    public:
        enum class ConnectionType {
            None,
            WiFi,
            Ethernet,
            Cellular
        };

        NetworkConnectionEvent(bool connected, ConnectionType type)
            : m_connected(connected), m_connectionType(type) {}

        bool isConnected() const { return m_connected; }
        ConnectionType getConnectionType() const { return m_connectionType; }

        std::string toString() const override {
            std::stringstream ss;
            ss << "NetworkConnectionEvent: " << (m_connected ? "connected" : "disconnected");
            if (m_connected) {
                const char* typeStr = "Unknown";
                switch (m_connectionType) {
                    case ConnectionType::None: typeStr = "None"; break;
                    case ConnectionType::WiFi: typeStr = "WiFi"; break;
                    case ConnectionType::Ethernet: typeStr = "Ethernet"; break;
                    case ConnectionType::Cellular: typeStr = "Cellular"; break;
                }
                ss << " (" << typeStr << ")";
            }
            return ss.str();
        }

        EVENT_CLASS_TYPE(NetworkConnectionChange)
        EVENT_CLASS_CATEGORY(EventCategoryEngine)
        EVENT_CLASS_PRIORITY(High)

    private:
        bool m_connected;
        ConnectionType m_connectionType;
    };

    /**
     * @brief System memory pressure event
     * 
     * Fired when system memory is running low.
     */
    class MemoryPressureEvent : public Event {
    public:
        enum class PressureLevel {
            Normal,
            Moderate,
            Critical
        };

        MemoryPressureEvent(PressureLevel level)
            : m_pressureLevel(level) {}

        PressureLevel getPressureLevel() const { return m_pressureLevel; }

        std::string toString() const override {
            std::stringstream ss;
            ss << "MemoryPressureEvent: ";
            switch (m_pressureLevel) {
                case PressureLevel::Normal: ss << "Normal"; break;
                case PressureLevel::Moderate: ss << "Moderate"; break;
                case PressureLevel::Critical: ss << "Critical"; break;
            }
            return ss.str();
        }

        EVENT_CLASS_TYPE(MemoryPressure)
        EVENT_CLASS_CATEGORY(EventCategoryEngine)
        EVENT_CLASS_PRIORITY(High)

    private:
        PressureLevel m_pressureLevel;
    };

    /**
     * @brief System theme change event
     * 
     * Fired when the system theme (light/dark) changes.
     */
    class SystemThemeChangeEvent : public Event {
    public:
        enum class Theme {
            Light,
            Dark
        };

        SystemThemeChangeEvent(Theme theme)
            : m_theme(theme) {}

        Theme getTheme() const { return m_theme; }

        std::string toString() const override {
            std::stringstream ss;
            ss << "SystemThemeChangeEvent: " << (m_theme == Theme::Light ? "Light" : "Dark");
            return ss.str();
        }

        EVENT_CLASS_TYPE(SystemThemeChange)
        EVENT_CLASS_CATEGORY(EventCategoryEngine)
        EVENT_CLASS_PRIORITY(Normal)

    private:
        Theme m_theme;
    };

    /**
     * @brief Storage space change event
     * 
     * Fired when available storage space changes significantly.
     */
    class StorageSpaceEvent : public Event {
    public:
        StorageSpaceEvent(uint64_t availableBytes, uint64_t totalBytes)
            : m_availableBytes(availableBytes), m_totalBytes(totalBytes) {}

        uint64_t getAvailableBytes() const { return m_availableBytes; }
        uint64_t getTotalBytes() const { return m_totalBytes; }
        float getUsagePercentage() const { 
            return m_totalBytes > 0 ? static_cast<float>(m_totalBytes - m_availableBytes) / m_totalBytes : 0.0f; 
        }

        std::string toString() const override {
            std::stringstream ss;
            ss << "StorageSpaceEvent: " << m_availableBytes << " / " << m_totalBytes 
               << " bytes (" << (getUsagePercentage() * 100) << "% used)";
            return ss.str();
        }

        EVENT_CLASS_TYPE(StorageSpaceChange)
        EVENT_CLASS_CATEGORY(EventCategoryEngine)
        EVENT_CLASS_PRIORITY(Normal)

    private:
        uint64_t m_availableBytes;
        uint64_t m_totalBytes;
    };

} // namespace AstralEngine

#endif // ASTRAL_ENGINE_SYSTEM_EVENT_H
