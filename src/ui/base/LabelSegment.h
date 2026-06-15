/*
 *  Arstro Artboard — LabelSegment: a reusable text visual node (input-transparent).
 */
#pragma once
#include "Segment.h"
#include "Theme.h"
#include <string>

namespace artboard
{
    class LabelSegment : public Segment
    {
    public:
        std::string text;
        TextStyle style;

    protected:
        void onPaint(IRenderTarget &t) const override;
        bool hitTestSelf(const Point &) const override { return false; }
    };
}
