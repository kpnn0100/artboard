/*
 *  Arstro Artboard — ProgressBar: the bar/level-meter *look* of a ProgressIndicator.
 *
 *  All progress state (clamped value, spring-smoothed display level, indeterminate
 *  phase, the four signals) lives in the ProgressIndicator base (FR-35); this class only
 *  paints. In indeterminate mode it sweeps a shuttle segment across the track instead of
 *  filling from the left, so "unknown progress" never reads as "no progress".
 */
#pragma once
#include "../base/ProgressIndicator.h"
#include "../base/Theme.h"

namespace artboard
{
    class ProgressBar : public ProgressIndicator
    {
    public:
        explicit ProgressBar(const ProgressStyle &style = Theme::basicTheme().progress);

        void setStyle(const ProgressStyle &style) { mStyle = style; }
        /** Width of the indeterminate shuttle as a fraction of the track (default 0.3). */
        void setShuttleFraction(double f) { mShuttle = f; }

    protected:
        void onPaint(IRenderTarget &t) const override;

    private:
        ProgressStyle mStyle;
        double mShuttle = 0.3;
    };
}
