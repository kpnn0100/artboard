/*
 *  Arstro Artboard — named motion tokens (FR-31).
 *
 *  A shared vocabulary of durations and Spring settle-speed presets so applications
 *  share one motion language instead of hand-picking ad hoc constants per control.
 *  Pure named data (header-only, like ui::Observable) -- no new primitive: durations
 *  are plain milliseconds consumed by Tween/AnimatedProperty, and the spring presets
 *  are just named `omega` values for the existing critically-damped Spring (FR-4d).
 *  Named cubic-bezier curves live alongside the rest of the easing library as
 *  Easing::Standard/StandardDecel/StandardAccel/EmphasizedDecel/EmphasizedAccel
 *  (Easing.h) rather than here, so there is one curve-lookup mechanism, not two.
 */
#pragma once

namespace artboard
{
    namespace motion
    {
        // ---- named duration scale (ms) ----
        constexpr double kDurationShort1 = 50.0;
        constexpr double kDurationShort2 = 100.0;
        constexpr double kDurationShort3 = 150.0;
        constexpr double kDurationShort4 = 200.0;
        constexpr double kDurationMedium1 = 250.0;
        constexpr double kDurationMedium2 = 300.0;
        constexpr double kDurationMedium3 = 350.0;
        constexpr double kDurationMedium4 = 400.0;
        constexpr double kDurationLong1 = 450.0;
        constexpr double kDurationLong2 = 500.0;
        constexpr double kDurationLong3 = 550.0;
        constexpr double kDurationLong4 = 600.0;

        // ---- named Spring settle-speed presets (omega, rad/s) ----
        // "Spatial" = position/size motion; "effects" = fades/colour (snappier defaults).
        // Larger omega settles faster (Spring::advance's existing default is 18 ~= 0.2s).
        constexpr double kSpatialFast = 26.0;
        constexpr double kSpatialDefault = 18.0;
        constexpr double kSpatialSlow = 10.0;
        constexpr double kEffectsFast = 40.0;
        constexpr double kEffectsDefault = 30.0;
        constexpr double kEffectsSlow = 20.0;
    }
}
