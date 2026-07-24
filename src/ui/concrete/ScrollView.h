/*
 *  Arstro Artboard — ScrollView: a clipped viewport over taller content
 *  (drag the body or the scrollbar thumb to scroll).
 */
#pragma once
#include "../base/Segment.h"
#include "../base/Theme.h"
#include "../../anim/Spring.h"
#include <memory>

namespace artboard
{
    class ScrollView : public Segment
    {
    public:
        explicit ScrollView(const ScrollStyle &style = Theme::basicTheme().scroll);

        void setContent(std::shared_ptr<Segment> content);
        void setContentHeight(double h) { mContentHeight = h; }
        double offset() const { return mOffset; }
        double maxOffset() const;
        void setStyle(const ScrollStyle &style) { mStyle = style; }

        void render(IRenderTarget &t, const Transform &parent = Transform::identity()) const override;
        void advance(double nowMs) override;

    protected:
        void onPaint(IRenderTarget &t) const override;
        bool handleGesture(const Gesture &g, const Point &localPoint) override;

    private:
        void syncContent() const;
        /** Rubber-band a raw (possibly out-of-range) offset: in range, passes through unchanged;
         *  past an edge, compresses the excess by kOverscrollFactor instead of hard-clamping
         *  (FR-29), so an active drag can pull slightly past the end with resistance. */
        double applyRubberBand(double raw) const;
        ScrollStyle mStyle;
        std::shared_ptr<Segment> mContent;
        double mContentHeight = 0.0;
        double mOffset = 0.0;
        bool mThumbDrag = false;
        bool mDragging = false;        // true from DragStart to Drop (thumb or body)
        double mDragStartOffset = 0.0;
        double mDragStartY = 0.0;
        double mLastMs = -1.0;
        Spring mScrollbar{0.0};  // scrollbar emphasis: eases up while hovering the viewport

        // ---- kinetic scrolling (FR-29) ----
        double mFlingVelocity = 0.0;   // px/s, offset-space; decays exponentially in advance()
        bool mSnapBackActive = false;  // true while mSnapBack is easing an out-of-range offset back
        Spring mSnapBack{0.0};         // eases mOffset to the nearest boundary once out of range
    };
}
