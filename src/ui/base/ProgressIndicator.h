/*
 *  Arstro Artboard — ProgressIndicator: the authorable base for determinate progress
 *  visuals (bars, rings, meters) — FR-35.
 *
 *  The determinate counterpart to VisualLoop (FR-34). It owns the *state* — a clamped
 *  [0,1] value, a spring-smoothed display level so the shown fill never jumps, an
 *  indeterminate mode with a free-running phase, and four signals — and draws nothing.
 *  `ProgressBar` is now one concrete look built on it; a Genesis-authored ring is
 *  another. This is the same separation `AbstractSlider`/`Slider` already make.
 *
 *  Non-interactive (input passes straight through): progress is a readout, not a control.
 */
#pragma once
#include "Segment.h"
#include "../../anim/Spring.h"
#include "../../anim/MotionTokens.h"

namespace artboard
{
    class ProgressIndicator : public Segment
    {
    public:
        ProgressIndicator() { inputTransparent = true; }

        /** The committed value in [0,1]. */
        double value() const { return mValue; }
        /** Set the value (clamped). The shown level eases toward it; fires
         *  onValueChanged(v), and onComplete() on the edge where it first reaches 1. */
        void setValue(double v);

        /** The spring-smoothed level actually drawn — always use this to paint. */
        double displayValue() const { return mDisplay.value(); }
        /** Settle speed of the display spring (rad/s); larger is snappier. */
        void setDisplayOmega(double omega) { mOmega = omega; }

        /** Indeterminate mode: the value is unknown, so `phase()` sweeps instead.
         *  Fires onIndeterminate() / onDeterminate() on change. */
        bool indeterminate() const { return mIndeterminate; }
        void setIndeterminate(bool on);

        /** Free-running sweep position in [0,1), advanced while indeterminate. */
        double phase() const { return mPhase; }
        /** Period of one indeterminate sweep in ms (default 1200). `<= 0` freezes phase. */
        void setPeriodMs(double ms) { mPeriodMs = ms; }
        double periodMs() const { return mPeriodMs; }

        void advance(double nowMs) override;

    protected:
        /** The value changed (already clamped). */
        virtual void onValueChanged(double v) { (void)v; }
        /** The value first reached 1 (edge-triggered; re-arms when it drops below 1). */
        virtual void onComplete() {}
        virtual void onIndeterminate() {}
        virtual void onDeterminate() {}

        bool hitTestSelf(const Point &) const override { return false; }
        double now() const { return mNowMs; }

    private:
        double mValue = 0.0;
        bool mIndeterminate = false;
        bool mCompleted = false;
        double mPhase = 0.0;
        double mPeriodMs = 1200.0;
        double mOmega = motion::kSpatialDefault;
        double mNowMs = 0.0;
        double mLastMs = -1.0;
        Spring mDisplay{0.0};
    };
}
