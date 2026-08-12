#include "CircleSegment.h"

namespace artboard
{
    void CircleSegment::onPaint(IRenderTarget &t) const
    {
        const double rx = width.value() * 0.5;
        const double ry = height.value() * 0.5;
        Path p = ellipsePath(rx, ry, rx, ry);
        if (trim.active())
            p = p.trimmed(trim.start, trim.end, trim.offset);   // FR-42: a trimmed circle is an arc
        p.paint = style.paint;
        p.emit(t);
    }
}
