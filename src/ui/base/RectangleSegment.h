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

    protected:
        void onPaint(IRenderTarget &t) const override;
    };
}
