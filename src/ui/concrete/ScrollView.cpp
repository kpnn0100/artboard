#include "ScrollView.h"
#include "../base/Interaction.h"

namespace artboard
{
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

    void ScrollView::clampOffset()
    {
        if (mOffset < 0.0)
            mOffset = 0.0;
        const double mx = maxOffset();
        if (mOffset > mx)
            mOffset = mx;
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
        if (g.type == T::DragStart)
        {
            mDragStartOffset = mOffset;
            mDragStartY = g.pos.y;
            mThumbDrag = localPoint.x > width.value() - 14.0;
            return true;
        }
        if (g.type == T::Drag)
        {
            const double dy = g.pos.y - mDragStartY;
            if (mThumbDrag)
            {
                const double scale = mContentHeight > 0.0 ? mContentHeight / height.value() : 1.0;
                mOffset = mDragStartOffset + dy * scale;
            }
            else
            {
                mOffset = mDragStartOffset - dy;
            }
            clampOffset();
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
        const double dt = mLastMs < 0.0 ? 0.0 : (nowMs - mLastMs) / 1000.0;
        mLastMs = nowMs;
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
