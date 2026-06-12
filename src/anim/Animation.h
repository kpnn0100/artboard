/*
 *  Arstro Artboard — abstract animation (device- and time-source-agnostic).
 *
 *  Animation is a pure tween (from -> to over a duration with an easing curve).
 *  AnimatedProperty wraps a live value that an Animation can drive over time; the
 *  caller advances it with update(nowMs). The same primitives serve live UI and
 *  offline video rendering.
 */
#pragma once
#include "Easing.h"

namespace artboard
{
    class Animation
    {
    public:
        Animation() = default;
        Animation(double from, double to, double durationMs, Easing easing = Easing::Linear)
            : mFrom(from), mTo(to), mDuration(durationMs), mEasing(easing) {}

        double value(double elapsedMs) const;
        bool finished(double elapsedMs) const { return elapsedMs >= mDuration; }
        double durationMs() const { return mDuration; }
        double to() const { return mTo; }

    private:
        double mFrom = 0, mTo = 0, mDuration = 0;
        Easing mEasing = Easing::Linear;
    };

    class AnimatedProperty
    {
    public:
        AnimatedProperty() = default;
        explicit AnimatedProperty(double value) : mValue(value) {}

        /** Snap to a value, cancelling any animation. */
        void set(double v) { mValue = v; mActive = false; }

        /** Begin animating from the current value to `target` over `durationMs`. */
        void animateTo(double target, double durationMs, Easing easing, double nowMs);

        /** Advance to time `nowMs`; returns the current value. */
        double update(double nowMs);

        double value() const { return mValue; }
        bool isAnimating() const { return mActive; }

    private:
        double mValue = 0;
        Animation mAnim;
        double mStart = 0;
        bool mActive = false;
    };
}
