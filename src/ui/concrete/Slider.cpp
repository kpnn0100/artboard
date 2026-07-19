#include "Slider.h"
#include "../base/Interaction.h"

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
        // Commit a deferred click-jump once the double-click guard has elapsed
        // without a second click cancelling it.
        if (mPendingClick)
        {
            if (mPendingSince < 0.0) mPendingSince = nowMs;
            else if (nowMs - mPendingSince >= mClickGuardMs)
            {
                mPendingClick = false;
                setValue(mPendingValue);
                if (onChange) onChange(value());
            }
        }
        double dt = mLastMs < 0.0 ? 0.0 : (nowMs - mLastMs) / 1000.0;
        mLastMs = nowMs;
        if (!mDisplayInit) { mDisplay.reset(value()); mDisplayInit = true; }
        mDisplay.setTarget(value());
        mDisplay.advance(dt); // shared critically-damped follower (~0.2s settle, matches Knob)
        // The reference reach eases to (value + offset) on its own follower, so both the
        // thumb and the reach end glide (no snap) when the value or the offset changes.
        if (!mSubDisplayInit) { mSubDisplay.reset(value() + mSubOffset); mSubDisplayInit = true; }
        mSubDisplay.setTarget(value() + mSubOffset);
        mSubDisplay.advance(dt);
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
        if (!mDisplayInit) { mDisplay.reset(value()); mDisplayInit = true; }
        const double span = maximum() - minimum();
        if (span <= 0.0) return 0.0;
        double n = (mDisplay.value() - minimum()) / span;
        return n < 0.0 ? 0.0 : (n > 1.0 ? 1.0 : n);
    }

    double Slider::subDisplayNormalized() const
    {
        if (!mSubDisplayInit) { mSubDisplay.reset(value() + mSubOffset); mSubDisplayInit = true; }
        const double span = maximum() - minimum();
        if (span <= 0.0) return 0.0;
        double n = (mSubDisplay.value() - minimum()) / span;
        return n < 0.0 ? 0.0 : (n > 1.0 ? 1.0 : n);
    }

    bool Slider::handleGesture(const Gesture &g, const Point &localPoint)
    {
        if (g.type == Gesture::Type::DoubleClick)
        {
            mPendingClick = false;   // cancel any deferred click-jump: reset wins cleanly
            resetToDefault();
            if (onChange)
                onChange(value());
            return true;
        }
        // A drag sets the value immediately (unambiguous). A click-to-position jump is
        // DEFERRED (see advance) so a double-click can cancel it — this prevents a
        // jump-to-cursor flash (and a wasted re-render) on double-click reset.
        if (g.type == Gesture::Type::Drag || g.type == Gesture::Type::DragStart)
        {
            mPendingClick = false;
            setValue(valueForLocalX(localPoint.x));
            if (onChange)
                onChange(value());
            return true;
        }
        if (g.type == Gesture::Type::Click && mClickJumps)
        {
            mPendingClick = true;                          // commit after the double-click guard
            mPendingValue = valueForLocalX(localPoint.x);
            mPendingSince = -1.0;
            return true;
        }
        if (g.type == Gesture::Type::Down)
        {
            // A new press cancels a still-deferred click-jump from the PREVIOUS
            // press — i.e. the second click of a double-click drops the first
            // click's pending jump before it can commit, so the reset stays clean.
            mPendingClick = false;
            return true;  // capture the press so a following drag is delivered here
        }
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
        self->mSubFill = std::make_shared<RectangleSegment>();
        self->mSubTick = std::make_shared<RectangleSegment>();
        self->mThumb = std::make_shared<CircleSegment>();
        self->mTrack->inputTransparent = true;
        self->mRangeFill->inputTransparent = true;
        self->mSubFill->inputTransparent = true;
        self->mSubTick->inputTransparent = true;
        self->mThumb->inputTransparent = true;
        self->addChild(self->mTrack);
        self->addChild(self->mRangeFill);
        self->addChild(self->mSubFill);   // reference reach, above the range fill
        self->addChild(self->mSubTick);
        self->addChild(self->mThumb);     // thumb on top of everything
    }

    void Slider::syncVisuals() const
    {
        ensureVisualTree();

        const double trackHeight = height.value() * 0.35;
        const double trackY = (height.value() - trackHeight) * 0.5;
        const double normalized = displayNormalized();  // spring-smoothed thumb/fill
        const double hv = hoverAmount();
        const double thumbDiameter = (mStyle.thumbRadius + 2.0 * hv) * 2.0;  // grows on hover
        const double thumbCenter = normalized * width.value();

        // A gradient track is drawn by onPaint; hide the solid track + range fill.
        mTrack->visible = !mHasGradient;
        mRangeFill->visible = !mHasGradient;

        mTrack->style = mStyle.track;
        mTrack->x.set(0.0);
        mTrack->y.set(trackY);
        mTrack->width.set(width.value());
        mTrack->height.set(trackHeight);

        // When the range spans zero, anchor the fill at the zero-crossing instead of
        // the left edge -- it then reads as "distance from neutral" (grows right for
        // positive values, left for negative) rather than "distance from minimum". A
        // range that doesn't span zero (e.g. 0..100) keeps filling from the left edge.
        double fillFrom = 0.0;
        if (minimum() < 0.0 && maximum() > 0.0)
        {
            const double span = maximum() - minimum();
            fillFrom = span > 0.0 ? (0.0 - minimum()) / span : 0.0;
        }
        const double fillLo = fillFrom < normalized ? fillFrom : normalized;
        const double fillHi = fillFrom < normalized ? normalized : fillFrom;

        mRangeFill->style = mStyle.rangeFill;
        mRangeFill->x.set(width.value() * fillLo);
        mRangeFill->y.set(trackY);
        mRangeFill->width.set(width.value() * (fillHi - fillLo));
        mRangeFill->height.set(trackHeight);

        // Secondary reference reach: a coloured section from the thumb to the reach end
        // (value + offset) plus a thin end tick. Both ease with their followers. Hidden
        // when there is no offset (thumb == reach). Works for a negative offset (reaches
        // left of the thumb) and over a gradient track alike.
        const bool showSub = mSubOffset != 0.0;
        mSubFill->visible = showSub;
        mSubTick->visible = showSub;
        if (showSub)
        {
            const double subNorm = subDisplayNormalized();
            const double reachLo = subNorm < normalized ? subNorm : normalized;
            const double reachHi = subNorm < normalized ? normalized : subNorm;
            mSubFill->style = {Paint::filled(mSubColor), trackHeight * 0.5};
            mSubFill->x.set(width.value() * reachLo);
            mSubFill->y.set(trackY);
            mSubFill->width.set(width.value() * (reachHi - reachLo));
            mSubFill->height.set(trackHeight);

            const double tickH = height.value() * 0.55, tickW = 1.6;
            mSubTick->style = {Paint::filled(mSubColor), 0.8};
            mSubTick->x.set(width.value() * subNorm - tickW * 0.5);
            mSubTick->y.set((height.value() - tickH) * 0.5);
            mSubTick->width.set(tickW);
            mSubTick->height.set(tickH);
        }

        mThumb->style = hoverBox(mStyle.thumb, mStyle.rangeFill.paint.fill, hv);
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
