#include "Geometry.h"
#include <cmath>

namespace artboard
{
    Transform Transform::rotation(double radians)
    {
        double cs = std::cos(radians), sn = std::sin(radians);
        return Transform{cs, sn, -sn, cs, 0, 0};
    }

    Transform Transform::mul(const Transform &o) const
    {
        // [a c e][o.a o.c o.e]
        // [b d f][o.b o.d o.f]
        return Transform{
            a * o.a + c * o.b,
            b * o.a + d * o.b,
            a * o.c + c * o.d,
            b * o.c + d * o.d,
            a * o.e + c * o.f + e,
            b * o.e + d * o.f + f};
    }
}
