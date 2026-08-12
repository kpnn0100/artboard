#include "ProgressIndicator.h"

namespace artboard
{
    void ProgressIndicator::setValue(double v)
    {
        const double clamped = v < 0.0 ? 0.0 : (v > 1.0 ? 1.0 : v);
        mValue = clamped;
        mDisplay.setTarget(clamped);  // the shown level eases toward it (never snaps)
        onValueChanged(clamped);
        if (clamped >= 1.0 && !mCompleted)
        {
            mCompleted = true;   // edge-triggered: onComplete fires once per arrival at 1
            onComplete();
        }
        else if (clamped < 1.0)
        {
            mCompleted = false;  // re-arm (a reused indicator can complete again)
        }
    }

    void ProgressIndicator::setIndeterminate(bool on)
    {
        if (on == mIndeterminate)
            return;
        mIndeterminate = on;
        if (on)
            onIndeterminate();
        else
            onDeterminate();
    }

    void ProgressIndicator::advance(double nowMs)
    {
        const double dt = mLastMs < 0.0 ? 0.0 : (nowMs - mLastMs) / 1000.0;
        mLastMs = nowMs;
        mNowMs = nowMs;
        mDisplay.advance(dt, mOmega);
        if (mIndeterminate && mPeriodMs > 0.0)
        {
            mPhase += dt * 1000.0 / mPeriodMs;
            mPhase -= (double)(long long)mPhase;  // wrap to [0,1)
        }
        Segment::advance(nowMs);
    }
}
