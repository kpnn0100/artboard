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
        /** Draw only part of the outline (FR-42): defaults draw all of it. Animating `end`
         *  from 0 to 1 makes the shape draw itself in; a circle trimmed to a sub-range is
         *  an arc. */
        Trim trim;
        /** Which part of the DISK this is (FR-43): a pie, a ring segment, or a pac-man.
         *  Distinct from `trim`, which says how much of the outline is drawn. */
        Arc arc;

    protected:
        void onPaint(IRenderTarget &t) const override;
    };
}
