/*
 *  Arstro Artboard — PathSegment: a freeform `Path` as a first-class interactive node
 *  (FR-37).
 *
 *  `RectangleSegment`, `CircleSegment`, and `LabelSegment` already wrap their shape as a
 *  Segment; an arbitrary path had no node type, so authored geometry (a spinner arc, a
 *  chevron, a logo mark) could not participate in layout, hover, opacity (FR-32), or the
 *  transform channel (FR-33). This closes that gap by reusing the existing `Path`
 *  drawable rather than re-implementing path storage.
 *
 *  The path is built in the segment's LOCAL space (origin at the segment's x/y), so it
 *  moves, rotates, scales, and fades with the segment like any other visual node.
 */
#pragma once
#include "Segment.h"
#include "../../scene/Shapes.h"

namespace artboard
{
    class PathSegment : public Segment
    {
    public:
        /** The geometry + paint. Build it with the usual Path chain:
         *      seg->path.moveTo(0,0).lineTo(10,0).cubicTo(...).close();
         *      seg->path.paint = Paint::stroked(c, 2.0);
         *  `Path::transform` stays available as an extra, path-local transform. */
        Path path;

        /** Drop every segment of the path (paint is kept) so it can be rebuilt. */
        void clearPath() { path.clear(); }

    protected:
        void onPaint(IRenderTarget &t) const override { path.emit(t); }
    };
}
