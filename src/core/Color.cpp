#include "Color.h"

namespace artboard
{
    Color Color::hex(uint32_t v, bool withAlpha)
    {
        if (withAlpha)
        {
            int a = (v >> 24) & 0xFF, r = (v >> 16) & 0xFF, g = (v >> 8) & 0xFF, b = v & 0xFF;
            return rgba(r, g, b, a);
        }
        int r = (v >> 16) & 0xFF, g = (v >> 8) & 0xFF, b = v & 0xFF;
        return rgba(r, g, b, 255);
    }
}
