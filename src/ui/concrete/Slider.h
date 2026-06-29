/*
 *  Arstro Artboard — Slider: a horizontal ranged control (track + fill + thumb).
 */
#pragma once
#include "../base/Segment.h"
#include "../base/Theme.h"
#include "../base/AbstractSlider.h"
#include "../base/RectangleSegment.h"
#include "../base/CircleSegment.h"
#include <functional>

namespace artboard
{
    class Slider : public Segment, public AbstractSlider
    {
    public:
        explicit Slider(const SliderStyle &style = Theme::basicTheme().slider);

        /** Fired when the user changes the value (drag / click / key / reset),
         *  not when setValue() is called programmatically (mirrors Knob). */
        std::function<void(double)> onChange;

        /** If true (default), a press jumps the value to the click position; if
         *  false, the value only changes on drag (so a click/double-click never
         *  hijacks it — double-click then reliably resets to the default). */
        void setClickJumps(bool jumps) { mClickJumps = jumps; }

        void setStyle(const SliderStyle &style);
        const SliderStyle &style() const { return mStyle; }
        void render(IRenderTarget &t, const Transform &parent = Transform::identity()) const override;
        /** Eases the displayed thumb/fill toward the target value each frame. */
        void advance(double nowMs) override;
        /** The spring-smoothed displayed value (lags the target during the glide). */
        double displayValue() const { if (!mDisplayInit) { mDisplay = value(); mDisplayInit = true; } return mDisplay; }

    protected:
        bool handleGesture(const Gesture &g, const Point &localPoint) override;
        bool handleKey(const KeyEvent &event) override;

    private:
        void ensureVisualTree() const;
        void syncVisuals() const;
        double valueForLocalX(double localX) const;
        double displayNormalized() const;  // spring-smoothed value mapped to [0,1]

        SliderStyle mStyle;
        bool mClickJumps = true;
        mutable std::shared_ptr<RectangleSegment> mTrack;
        mutable std::shared_ptr<RectangleSegment> mRangeFill;
        mutable std::shared_ptr<CircleSegment> mThumb;
        // Spring-smoothed display value (mirrors Knob): the thumb glides to the
        // target instead of snapping. mutable so onPaint can seed it pre-advance.
        mutable double mDisplay = 0.0;
        mutable bool mDisplayInit = false;
        double mVel = 0.0;
        double mLastMs = -1.0;
    };
}
