#include "Motion.h"

namespace artboard
{
    namespace
    {
        bool gReducedMotion = false;
    }

    void setReducedMotion(bool on) { gReducedMotion = on; }
    bool reducedMotion() { return gReducedMotion; }
}
