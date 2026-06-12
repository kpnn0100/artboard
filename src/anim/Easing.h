/*
 *  Arstro Artboard — easing curves (pure functions over t in [0,1]).
 */
#pragma once

namespace artboard
{
    enum class Easing
    {
        Linear,
        EaseInQuad,
        EaseOutQuad,
        EaseInOutCubic
    };

    /** Apply an easing curve. Input is clamped to [0,1]; output is in [0,1]. */
    double applyEasing(Easing e, double t);
}
