#pragma once
#include "../anim/Animation.h"

namespace artboard
{
    class Property
    {
    public:
        Property() = default;
        explicit Property(double value) : mAnimated(value) {}

        void set(double value) { mAnimated.set(value); }
        void animateTo(double target, double durationMs, Easing easing, double nowMs)
        {
            mAnimated.animateTo(target, durationMs, easing, nowMs);
        }
        double update(double nowMs) { return mAnimated.update(nowMs); }
        double value() const { return mAnimated.value(); }
        bool isAnimating() const { return mAnimated.isAnimating(); }

    private:
        AnimatedProperty mAnimated;
    };
}