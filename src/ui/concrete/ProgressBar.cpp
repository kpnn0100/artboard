#include "ProgressBar.h"

namespace artboard
{
    ProgressBar::ProgressBar(const ProgressStyle &style) : mStyle(style)
    {
        width.set(160.0);
        height.set(10.0);
    }

    void ProgressBar::onPaint(IRenderTarget &t) const
    {
        const double w = width.value(), h = height.value();
        drawRoundedRect(t, Rect{0, 0, w, h}, mStyle.track.cornerRadius, mStyle.track.paint);

        if (indeterminate())
        {
            // A shuttle sweeps left->right and wraps; clipped to the track so it never
            // spills past the ends (R3/§2A: unknown progress must still read as motion).
            const double sw = w * (mShuttle < 0.0 ? 0.0 : (mShuttle > 1.0 ? 1.0 : mShuttle));
            const double travel = w + sw;                 // enter from the left, exit right
            const double x = phase() * travel - sw;
            t.save();
            t.clipRect(0, 0, w, h);  // the shuttle enters and exits behind the track ends
            drawRoundedRect(t, Rect{x, 0, sw, h}, mStyle.fill.cornerRadius, mStyle.fill.paint);
            t.restore();
            return;
        }

        const double shown = displayValue();
        if (shown > 1e-4)  // near-zero draws no fill (also avoids a sub-pixel sliver)
            drawRoundedRect(t, Rect{0, 0, w * shown, h}, mStyle.fill.cornerRadius, mStyle.fill.paint);
    }
}
