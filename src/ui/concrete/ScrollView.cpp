#include "ScrollView.h"
#include "../base/Interaction.h"
#include "../../anim/MotionTokens.h"
#include <cmath>

namespace artboard
{
    namespace
    {
        constexpr double kOverscrollFactor = 0.35;   // fraction of raw excess shown once past an edge
        constexpr double kFlingFriction = 0.05;      // velocity multiplier per second (exponential decay)
        constexpr double kFlingStopVelocity = 20.0;  // px/s; below this, kinetic motion just stops
    }

    ScrollView::ScrollView(const ScrollStyle &style) : mStyle(style)
    {
        clipToBounds = true;
        width.set(200.0);
        height.set(200.0);
    }

    void ScrollView::setContent(std::shared_ptr<Segment> content)
    {
        clearChildren();
        mContent = content;
        if (content)
            addChild(content);
    }

    double ScrollView::maxOffset() const
    {
        const double m = mContentHeight - height.value();
        return m > 0.0 ? m : 0.0;
    }

    double ScrollView::applyRubberBand(double raw) const
    {
        const double lo = 0.0, hi = maxOffset();
        if (raw < lo) return lo + (raw - lo) * kOverscrollFactor;
        if (raw > hi) return hi + (raw - hi) * kOverscrollFactor;
        return raw;
    }

    void ScrollView::syncContent() const
    {
        if (mContent)
        {
            mContent->x.set(0.0);
            mContent->y.set(-mOffset);
        }
    }

    bool ScrollView::handleGesture(const Gesture &g, const Point &localPoint)
    {
        using T = Gesture::Type;
        if (g.type == T::Scroll)
        {
            // Wheel and drag land in the same place: the same clamp the drag path uses, and
            // the same kinetic state cleared, so the two cannot fight each other (FR-46).
            mFlingVelocity = 0.0;
            mSnapBackActive = false;
            const double maxOff = maxOffset();
            mOffset = std::min(maxOff, std::max(0.0, mOffset + g.delta.y));
            return maxOff > 0.0;   // nothing to scroll: let it bubble to something that can
        }
        if (g.type == T::DragStart)
        {
            mDragStartOffset = mOffset;
            mDragStartY = g.pos.y;
            mThumbDrag = localPoint.x > width.value() - 14.0;
            mDragging = true;
            mFlingVelocity = 0.0;    // a fresh grab cancels any residual kinetic motion
            mSnapBackActive = false; // and any in-progress snap-back
            return true;
        }
        if (g.type == T::Drag)
        {
            const double dy = g.pos.y - mDragStartY;
            double raw;
            if (mThumbDrag)
            {
                const double scale = mContentHeight > 0.0 ? mContentHeight / height.value() : 1.0;
                raw = mDragStartOffset + dy * scale;
            }
            else
            {
                raw = mDragStartOffset - dy;
            }
            mOffset = applyRubberBand(raw);
            return true;
        }
        if (g.type == T::Drop)
        {
            mDragging = false;
            return true;
        }
        if (g.type == T::Fling)
        {
            // Offset moves opposite the finger's Y (matching the Drag math above:
            // offset = start - dy), so the fling velocity is negated the same way.
            mFlingVelocity = -g.velocity.y;
            return true;
        }
        return Segment::handleGesture(g, localPoint);
    }

    void ScrollView::render(IRenderTarget &t, const Transform &parent) const
    {
        syncContent();
        Segment::render(t, parent);
    }

    void ScrollView::advance(double nowMs)
    {
        double dt = mLastMs < 0.0 ? 0.0 : (nowMs - mLastMs) / 1000.0;
        mLastMs = nowMs;
        if (dt > 0.05) dt = 0.05; // bound a long stall, matching Spring's own clamp

        if (!mDragging)
        {
            const double lo = 0.0, hi = maxOffset();
            if (mOffset < lo || mOffset > hi)
            {
                // Out of range (a released overscroll, or a fling that carried it past the edge):
                // kinetic motion stops and a Spring eases the offset back to the nearest boundary
                // (FR-29) -- the framework's existing glide-to-target primitive, not a new
                // hand-rolled per-frame integrator. Honors reduced motion for free (Spring::advance).
                const double target = mOffset < lo ? lo : hi;
                if (!mSnapBackActive)
                {
                    mSnapBack.reset(mOffset);
                    mSnapBack.setTarget(target);
                    mSnapBackActive = true;
                    mFlingVelocity = 0.0;
                }
                mOffset = mSnapBack.advance(dt, motion::kSpatialFast);
                if (!mSnapBack.isMoving())
                {
                    mOffset = target;
                    mSnapBackActive = false;
                }
            }
            else if (mFlingVelocity != 0.0)
            {
                mOffset += mFlingVelocity * dt;
                mFlingVelocity *= std::pow(kFlingFriction, dt);
                if (std::fabs(mFlingVelocity) < kFlingStopVelocity)
                    mFlingVelocity = 0.0;
            }
        }

        // Emphasise the scrollbar while the pointer is anywhere over the viewport (the
        // content child owns hover, so use hover-within, not this control's own hover).
        mScrollbar.setTarget(isHoverWithin() ? 1.0 : 0.0);
        mScrollbar.advance(dt);
        Segment::advance(nowMs);
    }

    void ScrollView::onPaint(IRenderTarget &t) const
    {
        const double w = width.value(), h = height.value();
        drawRoundedRect(t, Rect{0, 0, w, h}, mStyle.viewport.cornerRadius, mStyle.viewport.paint);
        if (maxOffset() > 0.0)
        {
            const double sb = mScrollbar.value();
            const double barW = 8.0 + 2.0 * sb;   // thumb widens slightly on hover
            const double barX = w - 2.0 - barW;
            drawRoundedRect(t, Rect{barX, 0, barW, h}, mStyle.track.cornerRadius, mStyle.track.paint);
            const double thumbH = h * (h / mContentHeight);
            const double thumbY = (mOffset / maxOffset()) * (h - thumbH);
            Paint thumb = mStyle.thumb.paint;
            thumb.fill = brighten(thumb.fill, interaction::kHoverFillLift * sb);  // and brightens
            drawRoundedRect(t, Rect{barX, thumbY, barW, thumbH}, mStyle.thumb.cornerRadius, thumb);
        }
    }
}
