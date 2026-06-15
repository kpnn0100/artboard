/*
 *  Arstro Artboard — ScrollView: a clipped viewport over taller content
 *  (drag the body or the scrollbar thumb to scroll).
 */
#pragma once
#include "../base/Segment.h"
#include "../base/Theme.h"
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

    protected:
        void onPaint(IRenderTarget &t) const override;
        bool handleGesture(const Gesture &g, const Point &localPoint) override;

    private:
        void syncContent() const;
        void clampOffset();
        ScrollStyle mStyle;
        std::shared_ptr<Segment> mContent;
        double mContentHeight = 0.0;
        double mOffset = 0.0;
        bool mThumbDrag = false;
        double mDragStartOffset = 0.0;
        double mDragStartY = 0.0;
    };
}
