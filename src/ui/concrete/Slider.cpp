#include "Slider.h"

namespace artboard
{
    Slider::Slider(const SliderStyle &style)
        : AbstractSlider(0.0, 0.0, 1.0), mStyle(style)
    {
        focusable = true;
        width.set(160.0);
        height.set(28.0);
    }

    void Slider::setStyle(const SliderStyle &style)
    {
        mStyle = style;
        syncVisuals();
    }

    void Slider::render(IRenderTarget &t, const Transform &parent) const
    {
        syncVisuals();
        Segment::render(t, parent);
    }

    void Slider::advance(double nowMs)
    {
        double dt = mLastMs < 0.0 ? 0.0 : (nowMs - mLastMs) / 1000.0;
        mLastMs = nowMs;
        if (!mDisplayInit) { mDisplay = value(); mDisplayInit = true; }
        if (dt > 0.0)
        {
            if (dt > 0.05) dt = 0.05;
            const double omega = 18.0;  // critically-damped spring, ~0.2s settle (matches Knob)
            const double acc = -2.0 * omega * mVel - omega * omega * (mDisplay - value());
            mVel += acc * dt;
            mDisplay += mVel * dt;
        }
        Segment::advance(nowMs);
    }

    void Slider::onPaint(IRenderTarget &t) const
    {
        if (!mHasGradient)
            return;
        const double trackHeight = height.value() * 0.35;
        const double y = (height.value() - trackHeight) * 0.5;
        const double w = width.value();
        const double r = trackHeight * 0.5;  // pill ends
        t.setLinearFill(0.0, 0.0, w, 0.0, mGradLeft, mGradRight);
        t.beginPath();
        t.moveTo(r, y);
        t.lineTo(w - r, y);
        t.quadTo(w, y, w, y + r);
        t.lineTo(w, y + trackHeight - r);
        t.quadTo(w, y + trackHeight, w - r, y + trackHeight);
        t.lineTo(r, y + trackHeight);
        t.quadTo(0.0, y + trackHeight, 0.0, y + trackHeight - r);
        t.lineTo(0.0, y + r);
        t.quadTo(0.0, y, r, y);
        t.closePath();
        t.fillPath();
    }

    double Slider::displayNormalized() const
    {
        if (!mDisplayInit) { mDisplay = value(); mDisplayInit = true; }
        const double span = maximum() - minimum();
        if (span <= 0.0) return 0.0;
        double n = (mDisplay - minimum()) / span;
        return n < 0.0 ? 0.0 : (n > 1.0 ? 1.0 : n);
    }

    bool Slider::handleGesture(const Gesture &g, const Point &localPoint)
    {
        if (g.type == Gesture::Type::DoubleClick)
        {
            resetToDefault(); // double-click restores the default value
            if (onChange)
                onChange(value());
            return true;
        }
        // Drag always sets the value; a bare press/click only jumps to the cursor
        // when click-jumps is enabled (off = the value only moves by dragging, so a
        // double-click never gets hijacked into a value change).
        const bool isDrag = g.type == Gesture::Type::Drag || g.type == Gesture::Type::DragStart;
        const bool isPress = g.type == Gesture::Type::Down || g.type == Gesture::Type::Click;
        if (isDrag || (isPress && mClickJumps))
        {
            setValue(valueForLocalX(localPoint.x));
            if (onChange)
                onChange(value());
            return true;
        }
        if (isPress)
            return true;  // capture the press so the following drag is delivered here
        return Segment::handleGesture(g, localPoint);
    }

    bool Slider::handleKey(const KeyEvent &event)
    {
        if (event.type != KeyEvent::Type::Down)
            return false;

        const double step = isAnalog() ? (maximum() - minimum()) / 20.0 : 1.0;
        if (event.keyCode == 37)
        {
            setValue(value() - step);
            if (onChange)
                onChange(value());
            return true;
        }
        if (event.keyCode == 39)
        {
            setValue(value() + step);
            if (onChange)
                onChange(value());
            return true;
        }
        return Segment::handleKey(event);
    }

    void Slider::ensureVisualTree() const
    {
        if (mTrack)
            return;

        auto self = const_cast<Slider *>(this);
        self->mTrack = std::make_shared<RectangleSegment>();
        self->mRangeFill = std::make_shared<RectangleSegment>();
        self->mThumb = std::make_shared<CircleSegment>();
        self->mTrack->inputTransparent = true;
        self->mRangeFill->inputTransparent = true;
        self->mThumb->inputTransparent = true;
        self->addChild(self->mTrack);
        self->addChild(self->mRangeFill);
        self->addChild(self->mThumb);
    }

    void Slider::syncVisuals() const
    {
        ensureVisualTree();

        const double trackHeight = height.value() * 0.35;
        const double trackY = (height.value() - trackHeight) * 0.5;
        const double normalized = displayNormalized();  // spring-smoothed thumb/fill
        const double thumbDiameter = mStyle.thumbRadius * 2.0;
        const double thumbCenter = normalized * width.value();

        // A gradient track is drawn by onPaint; hide the solid track + range fill.
        mTrack->visible = !mHasGradient;
        mRangeFill->visible = !mHasGradient;

        mTrack->style = mStyle.track;
        mTrack->x.set(0.0);
        mTrack->y.set(trackY);
        mTrack->width.set(width.value());
        mTrack->height.set(trackHeight);

        mRangeFill->style = mStyle.rangeFill;
        mRangeFill->x.set(0.0);
        mRangeFill->y.set(trackY);
        mRangeFill->width.set(width.value() * normalized);
        mRangeFill->height.set(trackHeight);

        mThumb->style = mStyle.thumb;
        mThumb->x.set(thumbCenter - thumbDiameter * 0.5);
        mThumb->y.set((height.value() - thumbDiameter) * 0.5);
        mThumb->width.set(thumbDiameter);
        mThumb->height.set(thumbDiameter);
    }

    double Slider::valueForLocalX(double localX) const
    {
        if (width.value() <= 0.0)
            return minimum();

        double normalized = localX / width.value();
        if (normalized < 0.0)
            normalized = 0.0;
        if (normalized > 1.0)
            normalized = 1.0;
        return minimum() + (maximum() - minimum()) * normalized;
    }
}
