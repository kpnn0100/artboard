/*
 *  Arstro Artboard — VisualLoop: the authorable base for indeterminate, looping
 *  visuals (spinners, busy pulses, loading screens) — FR-34.
 *
 *  It owns the *lifecycle*, never the picture: start/stop, a cycle clock, and three
 *  signals a subclass overrides to drive its own motion. `ProgressIndicator` (FR-35) is
 *  its determinate counterpart; the split is the same one `AbstractSlider`/`Slider`
 *  already make — "what the state means" lives apart from "what it looks like".
 *
 *  A bare VisualLoop draws nothing. Subclasses paint in onPaint() and animate their own
 *  Properties from the signals:
 *
 *      void onLoopStart() override {
 *          mRing->opacity.animate(Tween::range(0, 1, 300).withEasing(Easing::EaseOutCubic), now());
 *      }
 *
 *  Non-interactive by default (inputTransparent): a busy indicator is chrome, not a
 *  control. Platform-free — no HAL, no OS, no timing source of its own; the host's
 *  advance(nowMs) is the only clock.
 */
#pragma once
#include "Segment.h"

namespace artboard
{
    class VisualLoop : public Segment
    {
    public:
        VisualLoop() { inputTransparent = true; }

        /** Begin looping at `nowMs`; fires onLoopStart(). Idempotent while running. */
        void start(double nowMs);
        /** End the loop at `nowMs`; fires onLoopEnd(). No-op when not running. */
        void stop(double nowMs);
        bool running() const { return mRunning; }

        /** Length of one cycle in ms. Each completed cycle fires onCycle(index).
         *  `<= 0` disables cycle signals (the loop still runs). Default 1000 ms. */
        void setCycleMs(double ms) { mCycleMs = ms; }
        double cycleMs() const { return mCycleMs; }

        /** Completed cycles since the last start(). Reset by start(). */
        int cycleCount() const { return mCycles; }
        /** Progress through the current cycle in [0,1); 0 when stopped or cycleMs <= 0. */
        double cyclePhase() const;
        /** Milliseconds since start(); 0 when stopped. */
        double elapsedMs() const { return mRunning ? mNowMs - mStartMs : 0.0; }

        void advance(double nowMs) override;

    protected:
        /** The loop began. Kick off entry animations here. */
        virtual void onLoopStart() {}
        /** One full cycle completed; `index` counts from 1. Fires once per completed
         *  cycle even if several elapse in a single long frame, so counts stay exact. */
        virtual void onCycle(int index) { (void)index; }
        /** The loop ended. Run exit animations here. */
        virtual void onLoopEnd() {}

        /** The last time the host handed us (start/stop/advance) — the `nowMs` a
         *  subclass passes to Property::animate from inside a signal. */
        double now() const { return mNowMs; }

    private:
        bool mRunning = false;
        double mCycleMs = 1000.0;
        double mStartMs = 0.0;
        double mNowMs = 0.0;
        int mCycles = 0;
    };
}
