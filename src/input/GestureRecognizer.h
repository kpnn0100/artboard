/*
 *  Arstro Artboard — GestureRecognizer: raw pointer stream -> high-level gestures.
 *  Pure and platform-free, so the same click/drag/double-click semantics hold on
 *  every adapter. Feed RawPointer events; a sink receives synthesized Gestures.
 */
#pragma once
#include "Input.h"
#include <functional>

namespace artboard
{
    class GestureRecognizer
    {
    public:
        using Sink = std::function<void(const Gesture &)>;

        void setSink(Sink sink) { mSink = std::move(sink); }
        void setDragThreshold(double px) { mDragThreshold = px; }
        void setDoubleClickMs(double ms) { mDoubleClickMs = ms; }

        /** Feed one low-level pointer event; emits 0..n gestures to the sink. */
        void feed(const RawPointer &e);

    private:
        void emit(Gesture::Type t, const Point &pos, const Point &start, PointerButton b);

        Sink mSink;
        double mDragThreshold = 5.0;
        double mDoubleClickMs = 300.0;

        bool mPressed = false;
        bool mDragging = false;
        Point mDownPos;
        PointerButton mDownButton = PointerButton::Left;
        double mLastClickTime = -1e30;
        Point mLastClickPos;
    };
}
