/*
 *  Arstro Artboard — RectangleSegment: a reusable rounded-rectangle visual node.
 */
#pragma once
#include "Segment.h"
#include "Theme.h"

namespace artboard
{
    class RectangleSegment : public Segment
    {
    public:
        BoxStyle style;
        /** Draw only part of the outline (FR-42): defaults draw all of it. Animating `end`
         *  from 0 to 1 makes the shape draw itself in; a circle trimmed to a sub-range is
         *  an arc. */
        Trim trim;

    protected:
        void onPaint(IRenderTarget &t) const override;
    };
}
