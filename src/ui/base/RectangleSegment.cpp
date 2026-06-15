#include "RectangleSegment.h"

namespace artboard
{
    void RectangleSegment::onPaint(IRenderTarget &t) const
    {
        drawRoundedRect(t, localBounds(), style.cornerRadius, style.paint);
    }
}
