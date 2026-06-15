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
    }

    void ProgressBar::onPaint(IRenderTarget &t) const
    {
        const double w = width.value(), h = height.value();
        drawRoundedRect(t, Rect{0, 0, w, h}, mStyle.track.cornerRadius, mStyle.track.paint);
        if (mValue > 0.0)
            drawRoundedRect(t, Rect{0, 0, w * mValue, h}, mStyle.fill.cornerRadius, mStyle.fill.paint);
    }
}
