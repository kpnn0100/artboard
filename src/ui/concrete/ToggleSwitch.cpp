#include "ToggleSwitch.h"

namespace artboard
{
    ToggleSwitch::ToggleSwitch(const ToggleStyle &style) : mStyle(style)
    {
        focusable = true;
        width.set(48.0);
        height.set(26.0);
    }

    void ToggleSwitch::setOn(bool on)
    {
        mOn = on;
        mThumb.set(on ? 1.0 : 0.0);
    }

    void ToggleSwitch::toggle(double nowMs)
    {
        mOn = !mOn;
        mThumb.animateTo(mOn ? 1.0 : 0.0, 160.0, Easing::EaseOutCubic, nowMs);
        if (onChange)
            onChange(mOn);
    }

    void ToggleSwitch::advance(double nowMs)
    {
        mNowMs = nowMs;
        mThumb.update(nowMs);
        Segment::advance(nowMs);
    }

    void ToggleSwitch::onPaint(IRenderTarget &t) const
    {
        const double w = width.value(), h = height.value();
        const double t01 = mThumb.value();
        const BoxStyle &track = t01 >= 0.5 ? mStyle.trackOn : mStyle.trackOff;
        drawRoundedRect(t, Rect{0, 0, w, h}, track.cornerRadius, track.paint);

        const double r = h * 0.5 - 3.0;
        const double cx = h * 0.5 + t01 * (w - h);
        drawCircle(t, cx, h * 0.5, r, mStyle.thumb.paint);
    }

    bool ToggleSwitch::handleGesture(const Gesture &g, const Point &localPoint)
    {
        if (g.type == Gesture::Type::Click)
        {
            toggle(mNowMs);
            return true;
        }
        return Segment::handleGesture(g, localPoint);
    }

    bool ToggleSwitch::handleKey(const KeyEvent &event)
    {
        if (isConfirmKey(event))
        {
            toggle(mNowMs);
            return true;
        }
        return Segment::handleKey(event);
    }
}
