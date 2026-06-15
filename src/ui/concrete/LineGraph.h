/*
 *  Arstro Artboard — LineGraph: a non-interactive plot of a numeric series
 *  (grid + polyline + optional filled area) for data visualisation.
 */
#pragma once
#include "../base/Segment.h"
#include "../base/Theme.h"
#include <vector>

namespace artboard
{
    class LineGraph : public Segment
    {
    public:
        explicit LineGraph(const GraphStyle &style = Theme::basicTheme().graph);

        void setSeries(std::vector<double> values) { mSeries = std::move(values); }
        void setRange(double minimum, double maximum);
        void setFilled(bool filled) { mFilled = filled; }
        void setGridLines(int n) { mGridLines = n; }
        void setStyle(const GraphStyle &style) { mStyle = style; }

    protected:
        void onPaint(IRenderTarget &t) const override;
        bool hitTestSelf(const Point &) const override { return false; }

    private:
        GraphStyle mStyle;
        std::vector<double> mSeries;
        double mMin = -1.0;
        double mMax = 1.0;
        bool mFilled = true;
        int mGridLines = 3;
    };
}
