/*
 *  Arstro Artboard — LinearLayout: positions visible children along one axis with
 *  spacing + padding, and auto-sizes itself to the content. Base of Row / Column.
 */
#pragma once
#include "Segment.h"

namespace artboard
{
    class LinearLayout : public Segment
    {
    public:
        double spacing = 0.0;
        double padding = 0.0;

        void advance(double nowMs) override; // layout() then Segment::advance
        void layout();                        // position children + auto-size

    protected:
        explicit LinearLayout(bool horizontal) : mHorizontal(horizontal) {}

    private:
        bool mHorizontal;
    };
}
