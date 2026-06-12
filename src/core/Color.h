/*
 *  Arstro Artboard — Color (linear-agnostic RGBA, components 0..1).
 */
#pragma once
#include <cstdint>

namespace artboard
{
    struct Color
    {
        double r = 0, g = 0, b = 0, a = 1;
        Color() = default;
        Color(double r_, double g_, double b_, double a_ = 1.0) : r(r_), g(g_), b(b_), a(a_) {}

        /** 8-bit components 0..255. */
        static Color rgba(int r, int g, int b, int a = 255)
        {
            return Color{r / 255.0, g / 255.0, b / 255.0, a / 255.0};
        }
        /** 0xRRGGBB (opaque) or 0xAARRGGBB when withAlpha. */
        static Color hex(uint32_t v, bool withAlpha = false);

        bool operator==(const Color &o) const { return r == o.r && g == o.g && b == o.b && a == o.a; }
    };
}
