/*
 *  Arstro Artboard — input value types (platform-free).
 *
 *  RawPointer is the lowest-level fact an adapter reports (down/up/move + button
 *  + time). Gesture is what the GestureRecognizer synthesizes from that stream
 *  (click, double-click, right-click, drag, drop) — so adapters stay thin.
 */
#pragma once
#include "../core/Geometry.h"

namespace artboard
{
    enum class PointerButton { Left, Right, Middle };

    struct RawPointer
    {
        enum class Kind { Down, Up, Move };
        Kind kind;
        Point pos;
        PointerButton button = PointerButton::Left;
        double timeMs = 0.0;
        bool alt = false;    // Alt/Option modifier held (for alt-drag affordances)
        bool shift = false;  // Shift modifier (range selection)
        bool ctrl = false;   // Ctrl/Cmd modifier (toggle selection)
    };

    struct Gesture
    {
        enum class Type { Down, Up, Move, Click, DoubleClick, RightClick, DragStart, Drag, Drop };
        Type type;
        Point pos;                              // current position
        Point start;                            // press origin (for drags)
        PointerButton button = PointerButton::Left;
        bool alt = false;                       // Alt/Option modifier held at the event
        bool shift = false;                     // Shift modifier held at the event
        bool ctrl = false;                      // Ctrl/Cmd modifier held at the event
    };
}
