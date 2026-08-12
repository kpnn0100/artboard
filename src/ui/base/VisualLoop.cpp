#include "VisualLoop.h"

namespace artboard
{
    void VisualLoop::start(double nowMs)
    {
        if (mRunning)
            return;  // idempotent: re-starting a running loop must not re-fire onLoopStart
        mRunning = true;
        mStartMs = nowMs;
        mNowMs = nowMs;
        mCycles = 0;
        onLoopStart();
    }

    void VisualLoop::stop(double nowMs)
    {
        if (!mRunning)
            return;
        mNowMs = nowMs;
        mRunning = false;
        onLoopEnd();
    }

    double VisualLoop::cyclePhase() const
    {
        if (!mRunning || mCycleMs <= 0.0)
            return 0.0;
        const double t = (mNowMs - mStartMs) / mCycleMs;
        return t - (double)(long long)t;  // fractional part; elapsed is never negative
    }

    void VisualLoop::advance(double nowMs)
    {
        mNowMs = nowMs;
        if (mRunning && mCycleMs > 0.0)
        {
            // Fire once per completed cycle, even if a long frame spans several, so
            // cycleCount() is exact and a subclass keying off onCycle never skips a beat.
            const double elapsed = nowMs - mStartMs;
            const int completed = elapsed > 0.0 ? (int)(elapsed / mCycleMs) : 0;
            while (mCycles < completed)
                onCycle(++mCycles);
        }
        Segment::advance(nowMs);
    }
}
