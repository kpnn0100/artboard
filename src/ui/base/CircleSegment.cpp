#include "CircleSegment.h"

namespace artboard
{
    void CircleSegment::onPaint(IRenderTarget &t) const
    {
        const double k = 0.5522847498307936;
        const double rx = width.value() * 0.5;
        const double ry = height.value() * 0.5;
        const double cx = rx;
        const double cy = ry;
        const double ox = rx * k;
        const double oy = ry * k;

        t.beginPath();
        t.moveTo(cx - rx, cy);
        t.cubicTo(cx - rx, cy - oy, cx - ox, cy - ry, cx, cy - ry);
        t.cubicTo(cx + ox, cy - ry, cx + rx, cy - oy, cx + rx, cy);
        t.cubicTo(cx + rx, cy + oy, cx + ox, cy + ry, cx, cy + ry);
        t.cubicTo(cx - ox, cy + ry, cx - rx, cy + oy, cx - rx, cy);
        t.closePath();
        applyPaint(t, style.paint);
    }
}
