#include "LabelSegment.h"

namespace artboard
{
    void LabelSegment::onPaint(IRenderTarget &t) const
    {
        t.setFill(style.color);
        t.drawText(text, 0.0, style.sizePx, style.sizePx);
    }
}
