#include "Easing.h"
#include <cmath>

namespace artboard
{
    namespace
    {
        constexpr double kPi = 3.14159265358979323846;
        constexpr double c1 = 1.70158;       // back overshoot
        constexpr double c2 = c1 * 1.525;     // in-out back overshoot
        constexpr double c3 = c1 + 1.0;
        constexpr double c4 = (2.0 * kPi) / 3.0;
        constexpr double c5 = (2.0 * kPi) / 4.5;

        double outBounce(double t)
        {
            const double n1 = 7.5625, d1 = 2.75;
            if (t < 1.0 / d1)
                return n1 * t * t;
            if (t < 2.0 / d1)
            {
                t -= 1.5 / d1;
                return n1 * t * t + 0.75;
            }
            if (t < 2.5 / d1)
            {
                t -= 2.25 / d1;
                return n1 * t * t + 0.9375;
            }
            t -= 2.625 / d1;
            return n1 * t * t + 0.984375;
        }
    }

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
            return 1.0 - (1.0 - t) * (1.0 - t);
        case Easing::EaseInOutQuad:
            return t < 0.5 ? 2.0 * t * t
                           : 1.0 - (-2.0 * t + 2.0) * (-2.0 * t + 2.0) / 2.0;

        case Easing::EaseInCubic:
            return t * t * t;
        case Easing::EaseOutCubic:
            return 1.0 - std::pow(1.0 - t, 3.0);
        case Easing::EaseInOutCubic:
            return t < 0.5 ? 4.0 * t * t * t
                           : 1.0 - std::pow(-2.0 * t + 2.0, 3.0) / 2.0;

        case Easing::EaseInQuart:
            return t * t * t * t;
        case Easing::EaseOutQuart:
            return 1.0 - std::pow(1.0 - t, 4.0);
        case Easing::EaseInOutQuart:
            return t < 0.5 ? 8.0 * t * t * t * t
                           : 1.0 - std::pow(-2.0 * t + 2.0, 4.0) / 2.0;

        case Easing::EaseInSine:
            return 1.0 - std::cos((t * kPi) / 2.0);
        case Easing::EaseOutSine:
            return std::sin((t * kPi) / 2.0);
        case Easing::EaseInOutSine:
            return -(std::cos(kPi * t) - 1.0) / 2.0;

        case Easing::EaseInExpo:
            return t == 0.0 ? 0.0 : std::pow(2.0, 10.0 * t - 10.0);
        case Easing::EaseOutExpo:
            return t == 1.0 ? 1.0 : 1.0 - std::pow(2.0, -10.0 * t);
        case Easing::EaseInOutExpo:
            if (t == 0.0) return 0.0;
            if (t == 1.0) return 1.0;
            return t < 0.5 ? std::pow(2.0, 20.0 * t - 10.0) / 2.0
                           : (2.0 - std::pow(2.0, -20.0 * t + 10.0)) / 2.0;

        case Easing::EaseInBack:
            return c3 * t * t * t - c1 * t * t;
        case Easing::EaseOutBack:
            return 1.0 + c3 * std::pow(t - 1.0, 3.0) + c1 * std::pow(t - 1.0, 2.0);
        case Easing::EaseInOutBack:
            return t < 0.5
                       ? (std::pow(2.0 * t, 2.0) * ((c2 + 1.0) * 2.0 * t - c2)) / 2.0
                       : (std::pow(2.0 * t - 2.0, 2.0) * ((c2 + 1.0) * (2.0 * t - 2.0) + c2) + 2.0) / 2.0;

        case Easing::EaseInElastic:
            if (t == 0.0) return 0.0;
            if (t == 1.0) return 1.0;
            return -std::pow(2.0, 10.0 * t - 10.0) * std::sin((10.0 * t - 10.75) * c4);
        case Easing::EaseOutElastic:
            if (t == 0.0) return 0.0;
            if (t == 1.0) return 1.0;
            return std::pow(2.0, -10.0 * t) * std::sin((10.0 * t - 0.75) * c4) + 1.0;
        case Easing::EaseInOutElastic:
            if (t == 0.0) return 0.0;
            if (t == 1.0) return 1.0;
            return t < 0.5
                       ? -(std::pow(2.0, 20.0 * t - 10.0) * std::sin((20.0 * t - 11.125) * c5)) / 2.0
                       : (std::pow(2.0, -20.0 * t + 10.0) * std::sin((20.0 * t - 11.125) * c5)) / 2.0 + 1.0;

        case Easing::EaseInBounce:
            return 1.0 - outBounce(1.0 - t);
        case Easing::EaseOutBounce:
            return outBounce(t);
        case Easing::EaseInOutBounce:
            return t < 0.5 ? (1.0 - outBounce(1.0 - 2.0 * t)) / 2.0
                           : (1.0 + outBounce(2.0 * t - 1.0)) / 2.0;
        }
        return t; // defensive: only reached if `e` is an out-of-range cast
    }
}
