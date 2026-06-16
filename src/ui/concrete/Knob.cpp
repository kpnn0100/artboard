#include "Knob.h"
#include <cmath>

namespace artboard
{
    namespace
    {
        constexpr double kPi = 3.14159265358979323846;
        double knobSweepAngle(double v01) { return (135.0 + v01 * 270.0) * kPi / 180.0; }
    }

    Knob::Knob(const KnobStyle &style)
        : Segment(), AbstractSlider(0.0, 0.0, 1.0), mStyle(style)
    {
        focusable = true;
        width.set(64.0);
        height.set(64.0);
    }

    void Knob::emitChange()
    {
        if (onChange)
            onChange(value());
    }

    double Knob::displayNormalized() const
    {
        if (!mDisplayInit) { mDisplay = value(); mDisplayInit = true; }
        const double span = maximum() - minimum();
        if (span <= 0.0) return 0.0;
        double n = (mDisplay - minimum()) / span;
        return n < 0.0 ? 0.0 : (n > 1.0 ? 1.0 : n);
    }

    void Knob::advance(double nowMs)
    {
        double dt = mLastMs < 0.0 ? 0.0 : (nowMs - mLastMs) / 1000.0;
        mLastMs = nowMs;
        if (!mDisplayInit) { mDisplay = value(); mDisplayInit = true; }
        if (dt > 0.0)
        {
            if (dt > 0.05) dt = 0.05;
            const double omega = 18.0; // critically-damped; ~0.2s settle, continuous velocity
            const double acc = -2.0 * omega * mVel - omega * omega * (mDisplay - value());
            mVel += acc * dt;
            mDisplay += mVel * dt;
        }
        Segment::advance(nowMs);
    }

    void Knob::onPaint(IRenderTarget &t) const
    {
        const double w = width.value();
        const double avail = label.empty() ? height.value() : height.value() - 14.0;
        const double r = (w < avail ? w : avail) * 0.5 - 3.0;
        const double cx = w * 0.5;
        const double cy = avail * 0.5;
        const double arcR = r - mStyle.arcWidth;
        const double norm = displayNormalized();

        drawCircle(t, cx, cy, r, mStyle.dial.paint);

        // Full track arc (270° sweep).
        t.beginPath();
        for (int s = 0; s <= 24; ++s)
        {
            const double a = knobSweepAngle(s / 24.0);
            const Point p{cx + std::cos(a) * arcR, cy + std::sin(a) * arcR};
            if (s == 0)
                t.moveTo(p.x, p.y);
            else
                t.lineTo(p.x, p.y);
        }
        t.setStroke(mStyle.trackColor, mStyle.arcWidth);
        t.strokePath();

        // Value arc from start up to the current value.
        if (norm > 0.0)
        {
            const int steps = 1 + (int)(norm * 24.0);
            t.beginPath();
            for (int s = 0; s <= steps; ++s)
            {
                const double a = knobSweepAngle(norm * s / steps);
                const Point p{cx + std::cos(a) * arcR, cy + std::sin(a) * arcR};
                if (s == 0)
                    t.moveTo(p.x, p.y);
                else
                    t.lineTo(p.x, p.y);
            }
            t.setStroke(mStyle.valueColor, mStyle.arcWidth);
            t.strokePath();
        }

        // Indicator line — reaches the outer edge of the value arc
        // (arc radius + half the arc thickness).
        const double indR = arcR + mStyle.arcWidth * 0.5;
        const double ia = knobSweepAngle(norm);
        t.beginPath();
        t.moveTo(cx, cy);
        t.lineTo(cx + std::cos(ia) * indR, cy + std::sin(ia) * indR);
        t.setStroke(mStyle.indicatorColor, mStyle.arcWidth);
        t.strokePath();

        if (!label.empty())
        {
            t.setFill(mStyle.label.color);
            t.drawText(label, cx - label.size() * mStyle.label.sizePx * 0.3,
                       height.value() - 2.0, mStyle.label.sizePx);
        }
    }

    bool Knob::handleGesture(const Gesture &g, const Point &localPoint)
    {
        using T = Gesture::Type;
        if (g.type == T::DragStart)
        {
            mDragStartValue = value();
            return true;
        }
        if (g.type == T::Drag)
        {
            const double dy = g.start.y - g.pos.y; // drag up increases
            setValue(mDragStartValue + dy / sensitivity * (maximum() - minimum()));
            emitChange();
            return true;
        }
        return Segment::handleGesture(g, localPoint);
    }

    bool Knob::handleKey(const KeyEvent &event)
    {
        if (event.type == KeyEvent::Type::Down && (event.keyCode == 37 || event.keyCode == 39))
        {
            const double step = (maximum() - minimum()) / 20.0;
            setValue(value() + (event.keyCode == 39 ? step : -step));
            emitChange();
            return true;
        }
        return Segment::handleKey(event);
    }
}
