/*
 *  Arstro Artboard — Knob: a rotary analog control (vertical drag), onChange(value).
 */
#pragma once
#include "../base/Segment.h"
#include "../base/Theme.h"
#include "../base/AbstractSlider.h"
#include <functional>
#include <string>

namespace artboard
{
    class Knob : public Segment, public AbstractSlider
    {
    public:
        explicit Knob(const KnobStyle &style = Theme::basicTheme().knob);

        std::string label;
        double sensitivity = 160.0; // px of vertical drag for the full range
        std::function<void(double)> onChange;

        void setStyle(const KnobStyle &style) { mStyle = style; }
        const KnobStyle &style() const { return mStyle; }

        // Eases the displayed value toward the target each frame (smooth knob motion).
        void advance(double nowMs) override;

    protected:
        void onPaint(IRenderTarget &t) const override;
        bool handleGesture(const Gesture &g, const Point &localPoint) override;
        bool handleKey(const KeyEvent &event) override;

    private:
        void emitChange();
        double displayNormalized() const; // smoothed value mapped to [0,1]
        KnobStyle mStyle;
        double mDragStartValue = 0.0;
        // Smoothed display value (spring toward the real value); mutable so onPaint can
        // lazily seed it before the first advance().
        mutable double mDisplay = 0.0;
        mutable bool mDisplayInit = false;
        double mVel = 0.0;
        double mLastMs = -1.0;
    };
}
