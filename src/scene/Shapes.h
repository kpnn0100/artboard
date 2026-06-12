/*
 *  Arstro Artboard — concrete shapes. Each emits only path/text ops through the
 *  HAL (no rasterization), so every adapter draws them with the same primitives.
 */
#pragma once
#include "Drawable.h"
#include <vector>

namespace artboard
{
    /** Fill and/or stroke description. */
    struct Paint
    {
        bool hasFill = false;
        Color fill;
        bool hasStroke = false;
        Color stroke;
        double strokeWidth = 1.0;

        static Paint filled(const Color &c) { Paint p; p.hasFill = true; p.fill = c; return p; }
        static Paint stroked(const Color &c, double w) { Paint p; p.hasStroke = true; p.stroke = c; p.strokeWidth = w; return p; }
        static Paint filledStroked(const Color &f, const Color &s, double w)
        {
            Paint p = filled(f); p.hasStroke = true; p.stroke = s; p.strokeWidth = w; return p;
        }
    };

    /** Fill then stroke the current path according to `paint`. */
    void applyPaint(IRenderTarget &t, const Paint &paint);

    /** Rectangle / square, optional corner radius and border (via Paint). */
    class Rectangle : public Drawable
    {
    public:
        Rect rect;
        double cornerRadius = 0.0;
        Paint paint;
        Rectangle() = default;
        Rectangle(const Rect &r, const Paint &p, double radius = 0.0) : rect(r), paint(p), cornerRadius(radius) {}

    protected:
        void onDraw(IRenderTarget &t) const override;
    };

    /** A single stroked line segment. */
    class Line : public Drawable
    {
    public:
        Point a, b;
        Color color;
        double width = 1.0;
        Line() = default;
        Line(const Point &a_, const Point &b_, const Color &c, double w) : a(a_), b(b_), color(c), width(w) {}

    protected:
        void onDraw(IRenderTarget &t) const override;
    };

    /** Connected points; optionally closed (polygon). */
    class Polyline : public Drawable
    {
    public:
        std::vector<Point> points;
        bool closed = false;
        Paint paint;

    protected:
        void onDraw(IRenderTarget &t) const override;
    };

    /** Ellipse / circle (rx==ry), approximated with 4 cubic beziers. */
    class Ellipse : public Drawable
    {
    public:
        Point center;
        double rx = 0, ry = 0;
        Paint paint;
        Ellipse() = default;
        Ellipse(const Point &c, double rx_, double ry_, const Paint &p) : center(c), rx(rx_), ry(ry_), paint(p) {}

    protected:
        void onDraw(IRenderTarget &t) const override;
    };

    /** A free path: line / quadratic / cubic segments + a Catmull-Rom spline helper. */
    class Path : public Drawable
    {
    public:
        Paint paint;

        Path &moveTo(double x, double y);
        Path &lineTo(double x, double y);
        Path &quadTo(double cx, double cy, double x, double y);
        Path &cubicTo(double c1x, double c1y, double c2x, double c2y, double x, double y);
        Path &close();
        /** Append a smooth Catmull-Rom spline through `pts` (converted to cubics). */
        Path &spline(const std::vector<Point> &pts);

    protected:
        void onDraw(IRenderTarget &t) const override;

    private:
        struct Seg
        {
            enum class Op { Move, Line, Quad, Cubic, Close } op;
            double v[6] = {0, 0, 0, 0, 0, 0};
        };
        std::vector<Seg> mSegs;
    };

    /** A run of text at a baseline position. */
    class Text : public Drawable
    {
    public:
        std::string text;
        Point position;
        double sizePx = 16.0;
        Color color;
        Text() = default;
        Text(std::string s, const Point &pos, double size, const Color &c)
            : text(std::move(s)), position(pos), sizePx(size), color(c) {}

    protected:
        void onDraw(IRenderTarget &t) const override;
    };
}
