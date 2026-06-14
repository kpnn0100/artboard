#pragma once
#include "../input/Input.h"
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
}