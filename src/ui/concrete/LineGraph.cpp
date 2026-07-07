#include "LineGraph.h"

namespace artboard
{
    LineGraph::LineGraph(const GraphStyle &style) : mStyle(style)
    {
        width.set(240.0);
        height.set(120.0);
    }

    void LineGraph::setSeries(std::vector<double> values)
    {
        if (values.size() != mDisplay.size())
        {
            // Point count changed — a morph is ill-defined, so land on the new shape.
            mDisplay.assign(values.size(), Spring{});
            for (size_t i = 0; i < values.size(); ++i)
                mDisplay[i].reset(values[i]);
        }
        else
        {
            for (size_t i = 0; i < values.size(); ++i)
                mDisplay[i].setTarget(values[i]);  // morph toward the new value
        }
        mSeries = std::move(values);
    }

    void LineGraph::setRange(double minimum, double maximum)
    {
        mMin = minimum;
        mMax = maximum;
    }

    void LineGraph::advance(double nowMs)
    {
        const double dt = mLastMs < 0.0 ? 0.0 : (nowMs - mLastMs) / 1000.0;
        mLastMs = nowMs;
        for (auto &s : mDisplay)
            s.advance(dt, 26.0);  // snappy, so live/streaming data still tracks
        Segment::advance(nowMs);
    }

    void LineGraph::onPaint(IRenderTarget &t) const
    {
        const double w = width.value(), h = height.value();
        drawRoundedRect(t, Rect{0, 0, w, h}, mStyle.background.cornerRadius, mStyle.background.paint);

        for (int i = 1; i < mGridLines; ++i)
        {
            const double y = h * i / mGridLines;
            t.beginPath();
            t.moveTo(0.0, y);
            t.lineTo(w, y);
            t.setStroke(mStyle.gridColor, 1.0);
            t.strokePath();
        }

        if (mDisplay.size() < 2)
            return;

        double span = mMax - mMin;
        if (span <= 0.0)
            span = 1e-9;
        const int n = (int)mDisplay.size();
        auto valAt = [&](int i) { return mDisplay[i].value(); };  // the morphing shown value
        auto mapY = [&](double v) {
            double f = (v - mMin) / span;
            if (f < 0.0) f = 0.0;
            if (f > 1.0) f = 1.0;
            return h - f * h;
        };

        if (mFilled)
        {
            t.beginPath();
            t.moveTo(0.0, mapY(valAt(0)));
            for (int i = 1; i < n; ++i)
                t.lineTo(w * i / (n - 1), mapY(valAt(i)));
            t.lineTo(w, h);
            t.lineTo(0.0, h);
            t.closePath();
            t.setFill(mStyle.fillColor);
            t.fillPath();
        }

        t.beginPath();
        t.moveTo(0.0, mapY(valAt(0)));
        for (int i = 1; i < n; ++i)
            t.lineTo(w * i / (n - 1), mapY(valAt(i)));
        t.setStroke(mStyle.lineColor, mStyle.lineWidth);
        t.strokePath();
    }
}
