/*
 *  Arstro Artboard — LineGraph: a non-interactive plot of a numeric series
 *  (grid + polyline + optional filled area) for data visualisation.
 */
#pragma once
#include "../base/Segment.h"
#include "../base/Theme.h"
#include "../../anim/Spring.h"
#include <vector>

namespace artboard
{
    class LineGraph : public Segment
    {
    public:
        explicit LineGraph(const GraphStyle &style = Theme::basicTheme().graph);

        /** Set the target series; the plot morphs toward it (snaps only if the point
         *  count changes, since a morph across differing counts is ill-defined). */
        void setSeries(std::vector<double> values);
        void setRange(double minimum, double maximum);
        void setFilled(bool filled) { mFilled = filled; }
        void setGridLines(int n) { mGridLines = n; }
        void setStyle(const GraphStyle &style) { mStyle = style; }
        void advance(double nowMs) override;

    protected:
        void onPaint(IRenderTarget &t) const override;
        bool hitTestSelf(const Point &) const override { return false; }

    private:
        GraphStyle mStyle;
        std::vector<double> mSeries;    // target values
        std::vector<Spring> mDisplay;   // shown values, morph toward mSeries (fast omega)
        double mLastMs = -1.0;
        double mMin = -1.0;
        double mMax = 1.0;
        bool mFilled = true;
        int mGridLines = 3;
    };
}
