/*
 *  Arstro Artboard — easing curves (pure functions over t in [0,1]).
 *
 *  A curve maps normalized progress t -> eased progress. Endpoints are pinned:
 *  applyEasing(e, 0) == 0 and applyEasing(e, 1) == 1 for every curve. The
 *  overshoot families (Back, Elastic) may leave [0,1] *between* the endpoints —
 *  that overshoot is what gives motion its spring/snap. No backend code lives
 *  here, so the whole library is trivially unit-testable.
 */
#pragma once

namespace artboard
{
    enum class Easing
    {
        Linear,
        // quad
        EaseInQuad, EaseOutQuad, EaseInOutQuad,
        // cubic
        EaseInCubic, EaseOutCubic, EaseInOutCubic,
        // quart
        EaseInQuart, EaseOutQuart, EaseInOutQuart,
        // sine
        EaseInSine, EaseOutSine, EaseInOutSine,
        // expo
        EaseInExpo, EaseOutExpo, EaseInOutExpo,
        // back (overshoot)
        EaseInBack, EaseOutBack, EaseInOutBack,
        // elastic (springy overshoot)
        EaseInElastic, EaseOutElastic, EaseInOutElastic,
        // bounce
        EaseInBounce, EaseOutBounce, EaseInOutBounce,
        // named cubic-bezier curves (Material-3-style motion tokens, FR-31): control points
        // (x1,y1,x2,y2) matching CSS cubic-bezier() timing-function convention, solved
        // numerically rather than a closed-form formula (see Easing.cpp).
        Standard,          // cubic-bezier(0.2, 0.0, 0.0, 1.0)
        StandardDecel,     // cubic-bezier(0.0, 0.0, 0.0, 1.0)
        StandardAccel,     // cubic-bezier(0.3, 0.0, 1.0, 1.0)
        EmphasizedDecel,   // cubic-bezier(0.05, 0.7, 0.1, 1.0)
        EmphasizedAccel,   // cubic-bezier(0.3, 0.0, 0.8, 0.15)
        // The one curve whose shape its name does not fix (FR-4f): a cubic Hermite pinned to 0
        // and 1 whose endpoint slopes the caller supplies, so a leg of a chain can be made to
        // enter and leave at a chosen speed. Slopes are per-animation data, so they travel on the
        // Tween (or the applyEasing overload below) rather than in this enum.
        Hermite
    };

    /** Apply an easing curve. Input is clamped to [0,1]; endpoints map to 0 and 1.
     *  `Easing::Hermite` has no slopes of its own here, so it rests at both ends (smoothstep);
     *  pass authored slopes through the overload below. */
    double applyEasing(Easing e, double t);

    /**
     *  The cubic Hermite curve through (0,0) and (1,1) with h'(0) = slopeIn, h'(1) = slopeOut —
     *  the unique cubic those four facts determine (FR-4f). `t` is clamped to [0,1].
     *
     *  Slopes are dimensionless: eased progress per unit of normalized time. 1/1 reproduces
     *  Linear, 0/0 rests at both ends (smoothstep), and a large slope may carry the curve outside
     *  [0,1] between the endpoints — exactly as the back/elastic families already may. A caller
     *  animating a real quantity converts its own units once, outside this function.
     */
    double applyHermite(double t, double slopeIn, double slopeOut);

    /** `applyEasing` with authored endpoint slopes: `Easing::Hermite` applies them; every other
     *  curve has its shape fixed by definition and ignores them. */
    double applyEasing(Easing e, double t, double slopeIn, double slopeOut);
}
