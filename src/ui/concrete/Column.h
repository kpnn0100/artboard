/*
 *  Arstro Artboard — Column: a LinearLayout that arranges children top → bottom.
 */
#pragma once
#include "../base/LinearLayout.h"

namespace artboard
{
    class Column : public LinearLayout
    {
    public:
        Column() : LinearLayout(false) {}
    };
}
