#include "Knob.h"
#include <cmath>

namespace artboard
{
    namespace
    {
        constexpr double kPi = 3.14159265358979323846;
        double knobSweepAngle(double v01) { return (135.0 + v01 * 270.0) * kPi / 180.0; }
        double clamp01(double v) { return v < 0.0 ? 0.0 : (v > 1.0 ? 1.0 : v); }
        double clampPM1(double v) { return v < -1.0 ? -1.0 : (v > 1.0 ? 1.0 : v); }
    }

    double Knob::dialRadius() const
    {
        const double w = width.value();
        const double avail = label.empty() ? height.value() : height.value() - 14.0;
        return (w < avail ? w : avail) * 0.5 - 3.0;
    }

    int Knob::ringAtRadius(double rad) const
    {
        const double r = dialRadius();
        for (size_t i = 0; i < mMods.size(); ++i)
        {
            const double rr = r + 4.0 + (double)i * 5.0;
            if (rad >= rr - 2.5 && rad <= rr + 2.5)
                return (int)i;
        }
        return -1;
    }

    void Knob::addModulation(int sourceId, const Color &color, double depth, bool bipolar)
    {
        for (auto &m : mMods)
            if (m.sourceId == sourceId) { m.color = color; m.bipolar = bipolar; return; } // re-route (no re-animate)
        mMods.push_back(KnobMod{sourceId, clampPM1(depth), color, bipolar});
        mMods.back().appear.setTarget(1.0); // grow the ring in from zero depth
    }

    void Knob::setModDepth(int sourceId, double depth)
    {
        for (auto &m : mMods)
            if (m.sourceId == sourceId) { m.depth = clampPM1(depth); return; }
    }

    double Knob::modulatedValue() const
    {
        double v = value();
        if (mBus)
        {
            const double span = maximum() - minimum();
            for (const auto &m : mMods)
                v += m.depth * mBus->value(m.sourceId) * span;
        }
        if (v < minimum()) v = minimum();
        if (v > maximum()) v = maximum();
        return v;
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
        if (!mDisplayInit) { mDisplay.reset(value()); mDisplayInit = true; }
        const double span = maximum() - minimum();
        if (span <= 0.0) return 0.0;
        double n = (mDisplay.value() - minimum()) / span;
        return n < 0.0 ? 0.0 : (n > 1.0 ? 1.0 : n);
    }

    void Knob::advance(double nowMs)
    {
        double dt = mLastMs < 0.0 ? 0.0 : (nowMs - mLastMs) / 1000.0;
        mLastMs = nowMs;
        if (!mDisplayInit) { mDisplay.reset(value()); mDisplayInit = true; }
        mDisplay.setTarget(value());
        mDisplay.advance(dt); // shared critically-damped follower (~0.2s settle)
        for (auto &m : mMods)
            m.appear.advance(dt, 24.0); // ring grow-in, a touch snappier than the value
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

        // modulation depth rings (Serum-style), one concentric ring per routing.
        const double span = maximum() - minimum();
        const double liveNorm = span > 0.0 ? clamp01((modulatedValue() - minimum()) / span) : 0.0;
        for (size_t i = 0; i < mMods.size(); ++i)
        {
            const double rr = r + 4.0 + (double)i * 5.0;
            const double appear = mMods[i].appear.value(); // grow-in factor [0,1]
            // unipolar: arc base→base+depth; bipolar (LFO): arc base±|depth| (both directions)
            double n0, n1;
            if (mMods[i].bipolar)
            {
                const double d = (mMods[i].depth < 0 ? -mMods[i].depth : mMods[i].depth) * appear;
                n0 = clamp01(norm - d); n1 = clamp01(norm + d);
            }
            else
            {
                const double reach = clamp01(norm + mMods[i].depth * appear);
                n0 = norm < reach ? norm : reach; n1 = norm < reach ? reach : norm;
            }
            // depth arc from the base value to its reach, in the source colour
            t.beginPath();
            const int steps = 16;
            for (int s = 0; s <= steps; ++s)
            {
                const double a = knobSweepAngle(n0 + (n1 - n0) * s / steps);
                const Point p{cx + std::cos(a) * rr, cy + std::sin(a) * rr};
                if (s == 0) t.moveTo(p.x, p.y); else t.lineTo(p.x, p.y);
            }
            t.setStroke(mMods[i].color, 2.0);
            t.strokePath();
            // live dot at the current modulated value on this ring
            const double la = knobSweepAngle(liveNorm);
            drawCircle(t, cx + std::cos(la) * rr, cy + std::sin(la) * rr, 2.0, Paint::filled(mMods[i].color));
        }

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
        const double w = width.value();
        const double avail = label.empty() ? height.value() : height.value() - 14.0;
        const double dx = localPoint.x - w * 0.5, dy0 = localPoint.y - avail * 0.5;
        const int ring = ringAtRadius(std::sqrt(dx * dx + dy0 * dy0));

        if (g.type == T::Down)
        {
            mDragRing = ring; // -1 = dial (value drag), else a depth ring
            if (focusable) requestFocus();
            return true;
        }
        if (g.type == T::DoubleClick)
        {
            if (ring >= 0 && ring < (int)mMods.size()) // double-click a ring removes that routing
                mMods.erase(mMods.begin() + ring);
            else { resetToDefault(); emitChange(); }
            return true;
        }
        if (g.type == T::DragStart)
        {
            mDragStartValue = value();
            if (mDragRing >= 0 && mDragRing < (int)mMods.size())
                mDragStartDepth = mMods[mDragRing].depth;
            return true;
        }
        if (g.type == T::Drag)
        {
            const double dy = g.start.y - g.pos.y; // drag up increases
            if (mDragRing >= 0 && mDragRing < (int)mMods.size())
                setModDepth(mMods[mDragRing].sourceId, mDragStartDepth + dy / 120.0);
            else
            {
                setValue(mDragStartValue + dy / sensitivity * (maximum() - minimum()));
                emitChange();
            }
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
