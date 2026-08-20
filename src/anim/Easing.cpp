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

        // Solve the cubic bezier's parametric x(u) = t for u in [0,1], given P0=(0,0),
        // P1=(x1,_), P2=(x2,_), P3=(1,1). Newton-Raphson first (fast, matches the approach
        // browsers use for CSS cubic-bezier() timing functions); falls back to bisection when
        // the derivative is too small to trust (flat/near-vertical stretches of the curve).
        double cubicBezierSolveX(double x1, double x2, double t)
        {
            auto bezierX = [x1, x2](double u) {
                const double v = 1.0 - u;
                return 3.0 * v * v * u * x1 + 3.0 * v * u * u * x2 + u * u * u;
            };
            auto bezierDX = [x1, x2](double u) {
                const double v = 1.0 - u;
                return 3.0 * v * v * x1 + 6.0 * v * u * (x2 - x1) + 3.0 * u * u * (1.0 - x2);
            };
            double u = t; // identity is a reasonable initial guess for typical control points
            for (int i = 0; i < 8; ++i)
            {
                const double err = bezierX(u) - t;
                if (std::fabs(err) < 1e-7)
                    return u;
                const double d = bezierDX(u);
                if (std::fabs(d) < 1e-6)
                    break;
                u -= err / d;
                if (u < 0.0) u = 0.0;
                if (u > 1.0) u = 1.0;
            }
            double lo = 0.0, hi = 1.0;
            u = t;
            for (int i = 0; i < 30; ++i)
            {
                const double x = bezierX(u);
                if (std::fabs(x - t) < 1e-7)
                    break;
                if (x < t) lo = u; else hi = u;
                u = (lo + hi) / 2.0;
            }
            return u;
        }

        // y(u) at the u solving x(u) = t -- the cubic-bezier curve's eased output for input t.
        double cubicBezierY(double x1, double y1, double x2, double y2, double t)
        {
            if (t <= 0.0) return 0.0;
            if (t >= 1.0) return 1.0;
            const double u = cubicBezierSolveX(x1, x2, t);
            const double v = 1.0 - u;
            return 3.0 * v * v * u * y1 + 3.0 * v * u * u * y2 + u * u * u;
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

        case Easing::Standard:
            return cubicBezierY(0.2, 0.0, 0.0, 1.0, t);
        case Easing::StandardDecel:
            return cubicBezierY(0.0, 0.0, 0.0, 1.0, t);
        case Easing::StandardAccel:
            return cubicBezierY(0.3, 0.0, 1.0, 1.0, t);
        case Easing::EmphasizedDecel:
            return cubicBezierY(0.05, 0.7, 0.1, 1.0, t);
        case Easing::EmphasizedAccel:
            return cubicBezierY(0.3, 0.0, 0.8, 0.15, t);

        case Easing::Hermite:
            // No slopes were given, so the curve rests at both ends.
            return applyHermite(t, 0.0, 0.0);
        }
        return t; // defensive: only reached if `e` is an out-of-range cast
    }

    double applyHermite(double t, double slopeIn, double slopeOut)
    {
        if (t < 0.0) t = 0.0;
        if (t > 1.0) t = 1.0;
        // Hermite basis with values 0 at t=0 and 1 at t=1: the value terms collapse to the
        // smoothstep h01, and each tangent term carries one endpoint slope.
        const double t2 = t * t;
        const double t3 = t2 * t;
        return (3.0 * t2 - 2.0 * t3)          // value basis (0 at the start, 1 at the end)
               + slopeIn * (t3 - 2.0 * t2 + t) // h'(0) = slopeIn
               + slopeOut * (t3 - t2);         // h'(1) = slopeOut
    }

    double applyEasing(Easing e, double t, double slopeIn, double slopeOut)
    {
        // Only Hermite is shaped by the caller; every other curve's shape is its definition.
        if (e == Easing::Hermite)
            return applyHermite(t, slopeIn, slopeOut);
        return applyEasing(e, t);
    }
}
