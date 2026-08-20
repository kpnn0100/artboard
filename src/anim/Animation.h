/*
 *  Arstro Artboard — abstract animation (device- and time-source-agnostic).
 *
 *  Three layers, all pure w.r.t. time:
 *    Animation        — the original single-shot from->to tween (kept for compat).
 *    Tween            — a full spec: from/to + delay + easing + repeat + yoyo.
 *    AnimatedProperty — a live scalar driven by a Tween; advance with update(nowMs).
 *  The same primitives serve live UI and offline video rendering.
 */
#pragma once
#include "Easing.h"
#include <functional>

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

    /**
     *  A complete scalar animation as a pure function of elapsed time.
     *  Build it with aggregate init or the chainable factory helpers, e.g.
     *      Tween::range(0, 1, 300).withEasing(Easing::EaseOutCubic).looping()
     */
    struct Tween
    {
        double from = 0.0;
        double to = 0.0;
        double durationMs = 0.0;
        double delayMs = 0.0;
        Easing easing = Easing::Linear;
        int repeat = 0;     // extra cycles after the first; -1 = infinite
        bool yoyo = false;  // reverse direction on odd cycles
        // Endpoint slopes for Easing::Hermite (FR-4f), in eased progress per unit of normalized
        // time; 0/0 rests at both ends. Every other curve's shape is fixed, so it ignores them.
        double slopeIn = 0.0;
        double slopeOut = 0.0;

        Tween() = default;
        Tween(double from_, double to_, double durationMs_, double delayMs_ = 0.0,
              Easing easing_ = Easing::Linear, int repeat_ = 0, bool yoyo_ = false,
              double slopeIn_ = 0.0, double slopeOut_ = 0.0)
            : from(from_), to(to_), durationMs(durationMs_), delayMs(delayMs_),
              easing(easing_), repeat(repeat_), yoyo(yoyo_),
              slopeIn(slopeIn_), slopeOut(slopeOut_) {}

        static Tween range(double from_, double to_, double durationMs_)
        {
            return Tween(from_, to_, durationMs_);
        }
        Tween &withEasing(Easing e) { easing = e; return *this; }
        Tween &after(double ms) { delayMs = ms; return *this; }
        Tween &repeats(int n) { repeat = n; return *this; }
        Tween &looping() { repeat = -1; return *this; }
        Tween &yoyoing(bool y = true) { yoyo = y; return *this; }
        Tween &withSlopes(double in, double out) { slopeIn = in; slopeOut = out; return *this; }

        /** Sampled value at `elapsedMs` (delay + repeat + yoyo applied). */
        double at(double elapsedMs) const;
        /** delayMs + durationMs*(repeat+1); +inf when infinite. */
        double totalMs() const;
        /** false for infinite tweens; otherwise elapsedMs >= totalMs(). */
        bool finished(double elapsedMs) const;
    };

    class AnimatedProperty
    {
    public:
        AnimatedProperty() = default;
        explicit AnimatedProperty(double value) : mValue(value) {}

        /** Snap to a value, cancelling any animation (and pending onComplete). */
        void set(double v) { mValue = v; mActive = false; mOnComplete = nullptr; }

        /** Begin animating from the current value to `target` (single-shot). */
        void animateTo(double target, double durationMs, Easing easing, double nowMs);

        /** Drive the value from a full Tween spec; `onComplete` fires once at the end. */
        void animate(const Tween &spec, double nowMs, std::function<void()> onComplete = {});

        /** Advance to time `nowMs`; returns the current value. */
        double update(double nowMs);

        double value() const { return mValue; }
        bool isAnimating() const { return mActive; }

    private:
        double mValue = 0;
        Tween mTween;
        double mStart = 0;
        bool mActive = false;
        std::function<void()> mOnComplete;
    };
}
