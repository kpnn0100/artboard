#include "Animation.h"
#include <cmath>
#include <limits>

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

    double Tween::totalMs() const
    {
        if (repeat < 0)
            return std::numeric_limits<double>::infinity();
        return delayMs + durationMs * (repeat + 1);
    }

    bool Tween::finished(double elapsedMs) const
    {
        if (repeat < 0)
            return false;
        return elapsedMs >= totalMs();
    }

    double Tween::at(double elapsedMs) const
    {
        double local = elapsedMs - delayMs;
        if (local <= 0.0)
            return from; // still in the delay window (or before start)
        if (durationMs <= 0.0)
            return to; // instantaneous: snap once the delay elapses

        const bool infinite = repeat < 0;
        if (!infinite && local >= durationMs * (repeat + 1))
        {
            // Rest at the endpoint of the final cycle (yoyo flips odd cycles).
            const bool reversed = yoyo && (repeat % 2 == 1);
            return reversed ? from : to;
        }

        const double cycle = std::floor(local / durationMs);
        double phase = local / durationMs - cycle;
        const int index = static_cast<int>(cycle);
        if (yoyo && (index % 2 == 1))
            phase = 1.0 - phase;
        return from + (to - from) * applyEasing(easing, phase);
    }

    void AnimatedProperty::animateTo(double target, double durationMs, Easing easing, double nowMs)
    {
        mTween = Tween(mValue, target, durationMs, 0.0, easing);
        mStart = nowMs;
        mActive = true;
        mOnComplete = nullptr;
    }

    void AnimatedProperty::animate(const Tween &spec, double nowMs, std::function<void()> onComplete)
    {
        mTween = spec;
        mStart = nowMs;
        mActive = true;
        mOnComplete = std::move(onComplete);
        mValue = mTween.at(0.0);
    }

    double AnimatedProperty::update(double nowMs)
    {
        if (mActive)
        {
            double elapsed = nowMs - mStart;
            mValue = mTween.at(elapsed);
            if (mTween.finished(elapsed))
            {
                mActive = false;
                if (mOnComplete)
                {
                    auto cb = mOnComplete;
                    mOnComplete = nullptr;
                    cb();
                }
            }
        }
        return mValue;
    }
}
