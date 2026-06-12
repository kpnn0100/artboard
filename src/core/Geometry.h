/*
 *  Arstro Artboard — core geometry value types (platform-free).
 */
#pragma once

namespace artboard
{
    struct Point
    {
        double x = 0, y = 0;
        Point() = default;
        Point(double x_, double y_) : x(x_), y(y_) {}
        bool operator==(const Point &o) const { return x == o.x && y == o.y; }
    };

    struct Size
    {
        double w = 0, h = 0;
        Size() = default;
        Size(double w_, double h_) : w(w_), h(h_) {}
    };

    struct Rect
    {
        double x = 0, y = 0, w = 0, h = 0;
        Rect() = default;
        Rect(double x_, double y_, double w_, double h_) : x(x_), y(y_), w(w_), h(h_) {}
        double right() const { return x + w; }
        double bottom() const { return y + h; }
        bool contains(const Point &p) const { return p.x >= x && p.x <= right() && p.y >= y && p.y <= bottom(); }
    };

    /**
     * @brief 2D affine transform (Canvas2D convention):
     *   x' = a*x + c*y + e ,  y' = b*x + d*y + f
     */
    struct Transform
    {
        double a = 1, b = 0, c = 0, d = 1, e = 0, f = 0;

        static Transform identity() { return Transform{}; }
        static Transform translation(double tx, double ty) { return Transform{1, 0, 0, 1, tx, ty}; }
        static Transform scaling(double sx, double sy) { return Transform{sx, 0, 0, sy, 0, 0}; }
        static Transform rotation(double radians);

        /** this * other (apply `other` first, then `this`). */
        Transform mul(const Transform &o) const;
        Point apply(const Point &p) const { return Point{a * p.x + c * p.y + e, b * p.x + d * p.y + f}; }

        Transform &translate(double tx, double ty) { *this = mul(translation(tx, ty)); return *this; }
        Transform &scale(double sx, double sy) { *this = mul(scaling(sx, sy)); return *this; }
        Transform &rotate(double radians) { *this = mul(rotation(radians)); return *this; }
    };
}
