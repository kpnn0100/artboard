/*
 *  Arstro Artboard — Slider: a horizontal ranged control (track + fill + thumb).
 */
#pragma once
#include "../base/Segment.h"
#include "../base/Theme.h"
#include "../base/AbstractSlider.h"
#include "../base/RectangleSegment.h"
#include "../base/CircleSegment.h"
#include "../../anim/Spring.h"
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

        /** Render the track as a horizontal gradient (left value -> right value),
         *  e.g. a temperature blue->yellow ramp. Hides the solid track + range fill;
         *  the thumb still marks the position. Clear with a transparent pair. */
        void setTrackGradient(const Color &left, const Color &right) { mGradLeft = left; mGradRight = right; mHasGradient = true; }

        /** Secondary "reference reach": a fill of `subValueColor` from the thumb to
         *  (thumb value + offset), plus a thin end tick, easing with the thumb. `offset`
         *  is in value units (negative reaches left); 0 hides it. Used to show a value's
         *  effective total when an external contribution is added on top of the thumb
         *  (e.g. cosmo's group-stacked value). The thumb still marks the own value. */
        void setSubValueOffset(double offset) { mSubOffset = offset; }
        double subValueOffset() const { return mSubOffset; }
        void setSubValueColor(const Color &color) { mSubColor = color; }

        void setStyle(const SliderStyle &style);
        const SliderStyle &style() const { return mStyle; }
        void render(IRenderTarget &t, const Transform &parent = Transform::identity()) const override;
        /** Eases the displayed thumb/fill toward the target value each frame. */
        void advance(double nowMs) override;
        /** The spring-smoothed displayed value (lags the target during the glide). */
        double displayValue() const { if (!mDisplayInit) { mDisplay.reset(value()); mDisplayInit = true; } return mDisplay.value(); }

    protected:
        void onPaint(IRenderTarget &t) const override;  // gradient track (when set)
        bool handleGesture(const Gesture &g, const Point &localPoint) override;
        bool handleKey(const KeyEvent &event) override;

    private:
        void ensureVisualTree() const;
        void syncVisuals() const;
        double valueForLocalX(double localX) const;
        double displayNormalized() const;     // spring-smoothed value mapped to [0,1]
        double subDisplayNormalized() const;  // spring-smoothed (value + subOffset) mapped to [0,1]

        SliderStyle mStyle;
        bool mClickJumps = true;
        mutable std::shared_ptr<RectangleSegment> mTrack;
        mutable std::shared_ptr<RectangleSegment> mRangeFill;
        mutable std::shared_ptr<RectangleSegment> mSubFill;  // reference reach thumb -> thumb+offset
        mutable std::shared_ptr<RectangleSegment> mSubTick;  // thin line at the reach end
        mutable std::shared_ptr<CircleSegment> mThumb;
        double mSubOffset = 0.0;
        Color mSubColor{0.298, 0.710, 0.451, 1.0};  // #4cb573 default reference green
        mutable Spring mSubDisplay;
        mutable bool mSubDisplayInit = false;
        // Spring-smoothed display value (shared follower, mirrors Knob): the thumb
        // glides to the target instead of snapping. mutable so onPaint can seed it
        // pre-advance.
        mutable Spring mDisplay;
        mutable bool mDisplayInit = false;
        double mLastMs = -1.0;
        bool mHasGradient = false;
        Color mGradLeft, mGradRight;
        // A click-to-position jump is deferred so a double-click (reset) can cancel
        // it before it commits — no jump-to-cursor flash on double-click. The guard
        // MUST be >= the GestureRecognizer double-click window (default 300ms): a
        // shorter guard lets the jump commit between the two clicks of a slow
        // double-click, flashing the thumb toward the cursor before the reset. It is
        // also cancelled by the next press (the second click's Down), so the reset
        // stays clean regardless of exact timing.
        bool mPendingClick = false;
        double mPendingValue = 0.0;
        double mPendingSince = -1.0;
        double mClickGuardMs = 300.0;
    };
}
