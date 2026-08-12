#include "CircleSegment.h"

namespace artboard
{
    void CircleSegment::onPaint(IRenderTarget &t) const
    {
        const double rx = width.value() * 0.5;
        const double ry = height.value() * 0.5;
        // FR-43 picks the GEOMETRY (which part of the disk this is); FR-42 then says how much
        // of that geometry's outline is drawn. They compose: a trimmed pie is a pie whose
        // edge draws itself in.
        Path p = arc.active() ? ellipseArcPath(rx, ry, rx, ry, arc.start, arc.sweep, arc.innerRatio)
                              : ellipsePath(rx, ry, rx, ry);
        if (trim.active())
            p = p.trimmed(trim.start, trim.end, trim.offset);
        p.paint = style.paint;
        p.emit(t);
    }
}
