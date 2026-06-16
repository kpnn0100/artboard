#include "LinearLayout.h"

namespace artboard
{
    void LinearLayout::layout()
    {
        double cursor = padding; // main-axis running position
        double cross = 0.0;      // largest cross-axis extent
        bool any = false;
        for (const auto &child : children())
        {
            if (!child->visible)
                continue;
            any = true;
            if (mHorizontal)
            {
                child->x.set(cursor);
                child->y.set(padding);
                cursor += child->width.value() + spacing;
                if (child->height.value() > cross) cross = child->height.value();
            }
            else
            {
                child->y.set(cursor);
                child->x.set(padding);
                cursor += child->height.value() + spacing;
                if (child->width.value() > cross) cross = child->width.value();
            }
        }
        const double main = any ? (cursor - spacing + padding) : (padding * 2.0);
        const double crossSize = cross + padding * 2.0;
        if (mHorizontal)
        {
            width.set(main);
            height.set(crossSize);
        }
        else
        {
            height.set(main);
            width.set(crossSize);
        }
    }

    void LinearLayout::advance(double nowMs)
    {
        layout();
        Segment::advance(nowMs);
    }
}
