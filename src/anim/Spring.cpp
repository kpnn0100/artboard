#include "Spring.h"
#include "Motion.h"
#include <cmath>

namespace artboard
{
    bool Spring::isMoving(double eps) const
    {
        return std::fabs(mValue - mTarget) > eps || std::fabs(mVel) > eps;
    }

    double Spring::advance(double dtSeconds, double omega)
    {
        if (reducedMotion())
        {
            mValue = mTarget;
            mVel = 0.0;
            return mValue;
        }
        if (dtSeconds <= 0.0)
            return mValue;
        if (dtSeconds > 0.05)
            dtSeconds = 0.05; // bound a long stall so the step stays well-behaved

        // Closed-form critically-damped step (double root r = -omega). With
        // y = value - target: y(t) = (y0 + (v0 + omega*y0) t) e^(-omega t), and
        // y'(t) = (c2 - omega (y0 + c2 t)) e^(-omega t), c2 = v0 + omega*y0.
        // This is exact for any dt, so the trajectory is framerate-independent.
        const double y0 = mValue - mTarget;
        const double c2 = mVel + omega * y0;
        const double e = std::exp(-omega * dtSeconds);
        mValue = mTarget + (y0 + c2 * dtSeconds) * e;
        mVel = (c2 - omega * (y0 + c2 * dtSeconds)) * e;
        return mValue;
    }
}
