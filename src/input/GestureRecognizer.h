/*
 *  Arstro Artboard — GestureRecognizer: raw pointer stream -> high-level gestures.
 *  Pure and platform-free, so the same click/drag/double-click semantics hold on
 *  every adapter. Feed RawPointer events; a sink receives synthesized Gestures.
 */
#pragma once
#include "Input.h"
#include <functional>
#include <vector>

namespace artboard
{
    class GestureRecognizer
    {
    public:
        using Sink = std::function<void(const Gesture &)>;

        void setSink(Sink sink) { mSink = std::move(sink); }
        void setDragThreshold(double px) { mDragThreshold = px; }
        void setDoubleClickMs(double ms) { mDoubleClickMs = ms; }
        // FR-28 touch tuning:
        void setLongPressMs(double ms) { mLongPressMs = ms; }
        void setFlingVelocityThreshold(double pxPerSec) { mFlingThreshold = pxPerSec; }
        void setVelocityWindowMs(double ms) { mVelocityWindowMs = ms; }

        /** Feed one low-level pointer event; emits 0..n gestures to the sink. */
        void feed(const RawPointer &e);

        /** Time-tick (host calls once per frame, mirroring Segment/Spring/Animator): emits a
         *  LongPress if a non-dragging press has been held past longPressMs (FR-28). */
        void advance(double nowMs);

    private:
        void emit(Gesture::Type t, const Point &pos, const Point &start, PointerButton b,
                  const Point &velocity = Point{});

        Sink mSink;
        double mDragThreshold = 5.0;
        double mDoubleClickMs = 300.0;
        double mLongPressMs = 500.0;
        double mFlingThreshold = 400.0;
        double mVelocityWindowMs = 100.0;

        bool mAlt = false;  // modifier state of the event currently being processed
        bool mShift = false;
        bool mCtrl = false;
        bool mTouch = false;
        bool mPressed = false;
        bool mDragging = false;
        Point mDownPos;
        PointerButton mDownButton = PointerButton::Left;
        double mLastClickTime = -1e30;
        Point mLastClickPos;

        double mDownTimeMs = 0.0;
        bool mLongPressFired = false;
        struct VelocitySample { double timeMs; Point pos; };
        std::vector<VelocitySample> mVelocitySamples;
    };
}
