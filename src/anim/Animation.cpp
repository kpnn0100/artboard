#include "Animation.h"

namespace artboard
{
    double Animation::value(double elapsedMs) const
    {
        if (mDuration <= 0.0)
            return mTo; // zero-length tween: jump to target
        double t = elapsedMs / mDuration;
        if (t < 0.0) t = 0.0;
        if (t > 1.0) t = 1.0;
        return mFrom + (mTo - mFrom) * applyEasing(mEasing, t);
    }

    void AnimatedProperty::animateTo(double target, double durationMs, Easing easing, double nowMs)
    {
        mAnim = Animation(mValue, target, durationMs, easing);
        mStart = nowMs;
        mActive = true;
    }

    double AnimatedProperty::update(double nowMs)
    {
        if (mActive)
        {
            double elapsed = nowMs - mStart;
            mValue = mAnim.value(elapsed);
            if (mAnim.finished(elapsed))
            {
                mValue = mAnim.to();
                mActive = false;
            }
        }
        return mValue;
    }
}
