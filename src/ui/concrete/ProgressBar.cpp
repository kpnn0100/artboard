#include "ProgressBar.h"

namespace artboard
{
    ProgressBar::ProgressBar(const ProgressStyle &style) : mStyle(style)
    {
        width.set(160.0);
        height.set(10.0);
    }

    void ProgressBar::setValue(double v)
    {
        mValue = v < 0.0 ? 0.0 : (v > 1.0 ? 1.0 : v);
        mDisplay.setTarget(mValue);  // the shown level eases toward the new value
    }

    void ProgressBar::advance(double nowMs)
    {
        const double dt = mLastMs < 0.0 ? 0.0 : (nowMs - mLastMs) / 1000.0;
        mLastMs = nowMs;
        mDisplay.advance(dt);
        Segment::advance(nowMs);
    }

    void ProgressBar::onPaint(IRenderTarget &t) const
    {
        const double w = width.value(), h = height.value();
        const double shown = mDisplay.value();
        drawRoundedRect(t, Rect{0, 0, w, h}, mStyle.track.cornerRadius, mStyle.track.paint);
        if (shown > 1e-4)  // near-zero draws no fill (also avoids a sub-pixel sliver)
            drawRoundedRect(t, Rect{0, 0, w * shown, h}, mStyle.fill.cornerRadius, mStyle.fill.paint);
    }
}
