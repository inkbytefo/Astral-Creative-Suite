#pragma once

#include "Event.h"
#include <sstream>
#include <cstdint>

namespace AstralEngine {

    //=============================================================================
    // Keyboard Events
    //=============================================================================

    /**
     * @brief Base keyboard event
     */
    class KeyEvent : public Event {
    public:
        int getKeyCode() const { return m_keyCode; }

        EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)

    protected:
        KeyEvent(int keyCode) : m_keyCode(keyCode) {}

        int m_keyCode;
    };

    /**
     * @brief Key pressed event
     * 
     * Fired when a key is pressed. Includes repeat count for held keys.
     */
    class KeyPressedEvent : public KeyEvent {
    public:
        KeyPressedEvent(int keyCode, bool isRepeat = false)
            : KeyEvent(keyCode), m_isRepeat(isRepeat) {}

        bool isRepeat() const { return m_isRepeat; }

        std::string toString() const override {
            std::stringstream ss;
            ss << "KeyPressedEvent: " << m_keyCode << " (repeat: " << m_isRepeat << ")";
            return ss.str();
        }

        EVENT_CLASS_TYPE(KeyPressed)

    private:
        bool m_isRepeat;
    };

    /**
     * @brief Key released event
     * 
     * Fired when a key is released.
     */
    class KeyReleasedEvent : public KeyEvent {
    public:
        KeyReleasedEvent(int keyCode) : KeyEvent(keyCode) {}

        std::string toString() const override {
            std::stringstream ss;
            ss << "KeyReleasedEvent: " << m_keyCode;
            return ss.str();
        }

        EVENT_CLASS_TYPE(KeyReleased)
    };

    /**
     * @brief Key typed event
     * 
     * Fired when a character is typed (for text input).
     */
    class KeyTypedEvent : public KeyEvent {
    public:
        KeyTypedEvent(int keyCode) : KeyEvent(keyCode) {}

        std::string toString() const override {
            std::stringstream ss;
            ss << "KeyTypedEvent: " << m_keyCode;
            return ss.str();
        }

        EVENT_CLASS_TYPE(KeyTyped)
    };

    //=============================================================================
    // Mouse Events
    //=============================================================================

    /**
     * @brief Mouse moved event
     * 
     * Fired when the mouse cursor moves.
     */
    class MouseMovedEvent : public Event {
    public:
        MouseMovedEvent(float x, float y) : m_mouseX(x), m_mouseY(y) {}

        float getX() const { return m_mouseX; }
        float getY() const { return m_mouseY; }

        std::string toString() const override {
            std::stringstream ss;
            ss << "MouseMovedEvent: " << m_mouseX << ", " << m_mouseY;
            return ss.str();
        }

        EVENT_CLASS_TYPE(MouseMoved)
        EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)

    private:
        float m_mouseX, m_mouseY;
    };

    /**
     * @brief Mouse scrolled event
     * 
     * Fired when the mouse wheel is scrolled.
     */
    class MouseScrolledEvent : public Event {
    public:
        MouseScrolledEvent(float xOffset, float yOffset)
            : m_xOffset(xOffset), m_yOffset(yOffset) {}

        float getXOffset() const { return m_xOffset; }
        float getYOffset() const { return m_yOffset; }

        std::string toString() const override {
            std::stringstream ss;
            ss << "MouseScrolledEvent: " << m_xOffset << ", " << m_yOffset;
            return ss.str();
        }

        EVENT_CLASS_TYPE(MouseScrolled)
        EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)

    private:
        float m_xOffset, m_yOffset;
    };

    /**
     * @brief Base mouse button event
     */
    class MouseButtonEvent : public Event {
    public:
        int getMouseButton() const { return m_button; }

        EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)

    protected:
        MouseButtonEvent(int button) : m_button(button) {}

        int m_button;
    };

    /**
     * @brief Mouse button pressed event
     * 
     * Fired when a mouse button is pressed.
     */
    class MouseButtonPressedEvent : public MouseButtonEvent {
    public:
        MouseButtonPressedEvent(int button) : MouseButtonEvent(button) {}

        std::string toString() const override {
            std::stringstream ss;
            ss << "MouseButtonPressedEvent: " << m_button;
            return ss.str();
        }

        EVENT_CLASS_TYPE(MouseButtonPressed)
    };

    /**
     * @brief Mouse button released event
     * 
     * Fired when a mouse button is released.
     */
    class MouseButtonReleasedEvent : public MouseButtonEvent {
    public:
        MouseButtonReleasedEvent(int button) : MouseButtonEvent(button) {}

        std::string toString() const override {
            std::stringstream ss;
            ss << "MouseButtonReleasedEvent: " << m_button;
            return ss.str();
        }

        EVENT_CLASS_TYPE(MouseButtonReleased)
    };

    /**
     * @brief Mouse entered event
     * 
     * Fired when the mouse cursor enters the window.
     */
    class MouseEnteredEvent : public Event {
    public:
        MouseEnteredEvent() = default;

        std::string toString() const override {
            return "MouseEnteredEvent";
        }

        EVENT_CLASS_TYPE(MouseEntered)
        EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
    };

    /**
     * @brief Mouse left event
     * 
     * Fired when the mouse cursor leaves the window.
     */
    class MouseLeftEvent : public Event {
    public:
        MouseLeftEvent() = default;

        std::string toString() const override {
            return "MouseLeftEvent";
        }

        EVENT_CLASS_TYPE(MouseLeft)
        EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
    };

    //=============================================================================
    // Input State and Key/Button Codes
    //=============================================================================

    /**
     * @brief Key codes (based on SDL3 key codes for compatibility)
     */
    namespace KeyCode {
        enum : int {
            // Printable keys
            Space = 32,
            Apostrophe = 39, // '
            Comma = 44, // ,
            Minus = 45, // -
            Period = 46, // .
            Slash = 47, // /

            D0 = 48, // 0
            D1 = 49, // 1
            D2 = 50, // 2
            D3 = 51, // 3
            D4 = 52, // 4
            D5 = 53, // 5
            D6 = 54, // 6
            D7 = 55, // 7
            D8 = 56, // 8
            D9 = 57, // 9

            Semicolon = 59, // ;
            Equal = 61, // =

            A = 97, B = 98, C = 99, D = 100, E = 101, F = 102,
            G = 103, H = 104, I = 105, J = 106, K = 107, L = 108,
            M = 109, N = 110, O = 111, P = 112, Q = 113, R = 114,
            S = 115, T = 116, U = 117, V = 118, W = 119, X = 120,
            Y = 121, Z = 122,

            LeftBracket = 91,  // [
            Backslash = 92,    // backslash
            RightBracket = 93, // ]
            GraveAccent = 96,  // `

            // Function keys
            Escape = 256,
            Enter = 257,
            Tab = 258,
            Backspace = 259,
            Insert = 260,
            Delete = 261,
            Right = 262,
            Left = 263,
            Down = 264,
            Up = 265,
            PageUp = 266,
            PageDown = 267,
            Home = 268,
            End = 269,
            CapsLock = 280,
            ScrollLock = 281,
            NumLock = 282,
            PrintScreen = 283,
            Pause = 284,

            F1 = 290, F2 = 291, F3 = 292, F4 = 293, F5 = 294,
            F6 = 295, F7 = 296, F8 = 297, F9 = 298, F10 = 299,
            F11 = 300, F12 = 301,

            // Keypad
            KP0 = 320, KP1 = 321, KP2 = 322, KP3 = 323, KP4 = 324,
            KP5 = 325, KP6 = 326, KP7 = 327, KP8 = 328, KP9 = 329,
            KPDecimal = 330,
            KPDivide = 331,
            KPMultiply = 332,
            KPSubtract = 333,
            KPAdd = 334,
            KPEnter = 335,
            KPEqual = 336,

            // Modifiers
            LeftShift = 340,
            LeftControl = 341,
            LeftAlt = 342,
            LeftSuper = 343,
            RightShift = 344,
            RightControl = 345,
            RightAlt = 346,
            RightSuper = 347,
            Menu = 348
        };
    }

    /**
     * @brief Mouse button codes
     */
    namespace MouseButton {
        enum : int {
            Left = 0,
            Right = 1,
            Middle = 2,
            X1 = 3,
            X2 = 4
        };
    }

} // namespace AstralEngine
