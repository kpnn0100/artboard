/*
 *  Arstro Artboard — ProgressBar: non-interactive [0,1] bar / level meter.
 */
#pragma once
#include "../base/Segment.h"
#include "../base/Theme.h"
#include "../../anim/Spring.h"

namespace artboard
{
    class ProgressBar : public Segment
    {
    public:
        explicit ProgressBar(const ProgressStyle &style = Theme::basicTheme().progress);

        double value() const { return mValue; }
        void setValue(double v); // clamped to [0,1]; the shown level eases toward it
        void setStyle(const ProgressStyle &style) { mStyle = style; }
        void advance(double nowMs) override;

    protected:
        void onPaint(IRenderTarget &t) const override;
        bool hitTestSelf(const Point &) const override { return false; } // input passes through

    private:
        ProgressStyle mStyle;
        double mValue = 0.0;
        double mLastMs = -1.0;
        Spring mDisplay{0.0};  // shown level, eases toward mValue (no snapping)
    };
}
