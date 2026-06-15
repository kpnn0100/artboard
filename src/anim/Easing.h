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
        EaseInBounce, EaseOutBounce, EaseInOutBounce
    };

    /** Apply an easing curve. Input is clamped to [0,1]; endpoints map to 0 and 1. */
    double applyEasing(Easing e, double t);
}
