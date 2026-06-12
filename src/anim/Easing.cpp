#include "Easing.h"

namespace artboard
{
    double applyEasing(Easing e, double t)
    {
        if (t < 0.0) t = 0.0;
        if (t > 1.0) t = 1.0;
        switch (e)
        {
        case Easing::Linear:
            return t;
        case Easing::EaseInQuad:
            return t * t;
        case Easing::EaseOutQuad:
            return t * (2.0 - t);
        case Easing::EaseInOutCubic:
            return t < 0.5 ? 4.0 * t * t * t
                           : 1.0 - (-2.0 * t + 2.0) * (-2.0 * t + 2.0) * (-2.0 * t + 2.0) / 2.0;
        }
        return t; // unreachable for the enum; defensive
    }
}
