/*
 *  Arstro Artboard — Row: a LinearLayout that arranges children left → right.
 */
#pragma once
#include "../base/LinearLayout.h"

namespace artboard
{
    class Row : public LinearLayout
    {
    public:
        Row() : LinearLayout(true) {}
    };
}
