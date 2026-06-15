#pragma once
#include "../../input/Input.h"
#include <string>

namespace artboard
{
    class Segment;

    struct KeyEvent
    {
        enum class Type { Down, Up, Text };
        Type type = Type::Down;
        int keyCode = 0;
        std::string text;
        bool shift = false;
        bool ctrl = false;
        bool alt = false;
    };

    class InputController
    {
    public:
        virtual ~InputController() = default;
        virtual bool onGesture(Segment &segment, const Gesture &gesture, const Point &localPoint) = 0;
        virtual bool onKey(Segment &segment, const KeyEvent &event) = 0;
    };

    /** Enter (13) or Space (32) key-down — the shared "confirm/activate" gesture. */
    inline bool isConfirmKey(const KeyEvent &event)
    {
        return event.type == KeyEvent::Type::Down && (event.keyCode == 13 || event.keyCode == 32);
    }
}