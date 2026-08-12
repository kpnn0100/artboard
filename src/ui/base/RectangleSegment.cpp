#include "RectangleSegment.h"

namespace artboard
{
    void RectangleSegment::onPaint(IRenderTarget &t) const
    {
        Path p = roundedRectPath(localBounds(), style.cornerRadius);
        if (trim.active())
            p = p.trimmed(trim.start, trim.end, trim.offset);   // FR-42
        p.paint = style.paint;
        p.emit(t);
    }
}
