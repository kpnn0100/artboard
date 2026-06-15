/*
 *  Arstro Artboard — CircleSegment: a reusable ellipse/circle visual node sized to
 *  the segment's bounds (rx = width/2, ry = height/2).
 */
#pragma once
#include "Segment.h"
#include "Theme.h"

namespace artboard
{
    class CircleSegment : public Segment
    {
    public:
        BoxStyle style;

    protected:
        void onPaint(IRenderTarget &t) const override;
    };
}
