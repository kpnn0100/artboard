/*
 *  Arstro Artboard — ToggleSwitch: animated boolean switch, onChange(bool).
 */
#pragma once
#include "../base/Segment.h"
#include "../base/Theme.h"
#include <functional>

namespace artboard
{
    class ToggleSwitch : public Segment
    {
    public:
        explicit ToggleSwitch(const ToggleStyle &style = Theme::basicTheme().toggle);

        std::function<void(bool)> onChange;

        bool on() const { return mOn; }
        void setOn(bool on);                 // snap, no animation, no callback
        void toggle(double nowMs);           // animate + fire onChange

        void setStyle(const ToggleStyle &style) { mStyle = style; }
        void advance(double nowMs) override;  // ticks the thumb animation

    protected:
        void onPaint(IRenderTarget &t) const override;
        bool handleGesture(const Gesture &g, const Point &localPoint) override;
        bool handleKey(const KeyEvent &event) override;

    private:
        ToggleStyle mStyle;
        bool mOn = false;
        double mNowMs = 0.0;
        AnimatedProperty mThumb{0.0}; // 0 = off position, 1 = on position
    };
}
