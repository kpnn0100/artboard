/*
 *  Arstro Artboard — Spring: a critically-damped, framerate-independent follower
 *  for display smoothing (FR-4d).
 *
 *  A value that should *glide* to a target rather than snap (a knob dial, a slider
 *  thumb, a morphing wave) advances with `advance(dtSeconds, omega)`. The step uses
 *  the closed-form critically-damped solution, so advancing one big step yields the
 *  same trajectory as many small ones — the smoothing does not change with frame
 *  rate. Critically damped means it never overshoots. This is the single source of
 *  truth for the per-frame Euler integrators that were previously hand-copied into
 *  individual controls. Honors the global reduced-motion switch (Motion.h).
 */
#pragma once

namespace artboard
{
    class Spring
    {
    public:
        Spring() = default;
        explicit Spring(double value) : mValue(value), mTarget(value) {}

        /** Snap value and target to `v`, clearing velocity. */
        void reset(double v) { mValue = v; mTarget = v; mVel = 0.0; }
        void setTarget(double t) { mTarget = t; }
        double target() const { return mTarget; }
        double value() const { return mValue; }
        double velocity() const { return mVel; }

        /** True while the value/velocity are still meaningfully away from the target. */
        bool isMoving(double eps = 1e-3) const;

        /** Advance toward the target by a real time delta. `omega` (rad/s) sets the
         *  settle speed (larger = snappier; 18 ≈ 0.2s). Returns the new value. */
        double advance(double dtSeconds, double omega = 18.0);

    private:
        double mValue = 0.0;
        double mTarget = 0.0;
        double mVel = 0.0;
    };
}
