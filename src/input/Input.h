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
        enum class Kind { Down, Up, Move, Scroll };
        Kind kind;
        Point pos;
        PointerButton button = PointerButton::Left;
        double timeMs = 0.0;
        bool alt = false;    // Alt/Option modifier held (for alt-drag affordances)
        bool shift = false;  // Shift modifier (range selection)
        bool ctrl = false;   // Ctrl/Cmd modifier (toggle selection)
        bool touch = false;  // true for a touchscreen source, false for mouse/pointer (FR-28)
        /** Scroll only (FR-46): the delta in PIXELS. `y > 0` scrolls toward the end (the
         *  content moves up), matching a wheel pushed down. Pixels rather than notches because
         *  only the host knows whether it has a stepped wheel or a continuous trackpad.
         *  Last member deliberately, so existing brace-initialisation keeps working. */
        Point scroll;
    };

    struct Gesture
    {
        enum class Type { Down, Up, Move, Click, DoubleClick, RightClick, DragStart, Drag, Drop,
                           LongPress, Fling, Scroll };
        Type type;
        Point pos;                              // current position
        Point start;                            // press origin (for drags)
        PointerButton button = PointerButton::Left;
        bool alt = false;                       // Alt/Option modifier held at the event
        bool shift = false;                     // Shift modifier held at the event
        bool ctrl = false;                      // Ctrl/Cmd modifier held at the event
        bool touch = false;                     // carried from RawPointer::touch (FR-28)
        Point velocity;                         // px/s; set only on Fling, {0,0} otherwise
        Point delta;                            // px; set only on Scroll (FR-46), {0,0} otherwise
    };
}
