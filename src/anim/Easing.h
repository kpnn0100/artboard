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
        EmphasizedAccel    // cubic-bezier(0.3, 0.0, 0.8, 0.15)
    };

    /** Apply an easing curve. Input is clamped to [0,1]; endpoints map to 0 and 1. */
    double applyEasing(Easing e, double t);
}
