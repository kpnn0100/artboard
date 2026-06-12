#include "Shapes.h"

namespace artboard
{
    void applyPaint(IRenderTarget &t, const Paint &paint)
    {
        if (paint.hasFill)
        {
            t.setFill(paint.fill);
            t.fillPath();
        }
        if (paint.hasStroke)
        {
            t.setStroke(paint.stroke, paint.strokeWidth);
            t.strokePath();
        }
    }

    void Rectangle::onDraw(IRenderTarget &t) const
    {
        const double x = rect.x, y = rect.y, w = rect.w, h = rect.h;
        t.beginPath();
        if (cornerRadius > 0.0)
        {
            // Clamp radius to half the smaller side.
            double r = cornerRadius;
            double half = (w < h ? w : h) * 0.5;
            if (r > half) r = half;
            t.moveTo(x + r, y);
            t.lineTo(x + w - r, y);     t.quadTo(x + w, y, x + w, y + r);
            t.lineTo(x + w, y + h - r); t.quadTo(x + w, y + h, x + w - r, y + h);
            t.lineTo(x + r, y + h);     t.quadTo(x, y + h, x, y + h - r);
            t.lineTo(x, y + r);         t.quadTo(x, y, x + r, y);
            t.closePath();
        }
        else
        {
            t.moveTo(x, y);
            t.lineTo(x + w, y);
            t.lineTo(x + w, y + h);
            t.lineTo(x, y + h);
            t.closePath();
        }
        applyPaint(t, paint);
    }

    void Line::onDraw(IRenderTarget &t) const
    {
        t.beginPath();
        t.moveTo(a.x, a.y);
        t.lineTo(b.x, b.y);
        t.setStroke(color, width);
        t.strokePath();
    }

    void Polyline::onDraw(IRenderTarget &t) const
    {
        if (points.empty())
            return;
        t.beginPath();
        t.moveTo(points[0].x, points[0].y);
        for (size_t i = 1; i < points.size(); ++i)
            t.lineTo(points[i].x, points[i].y);
        if (closed)
            t.closePath();
        applyPaint(t, paint);
    }

    void Ellipse::onDraw(IRenderTarget &t) const
    {
        const double k = 0.5522847498307936; // cubic bezier circle constant
        const double cx = center.x, cy = center.y;
        const double ox = rx * k, oy = ry * k;
        t.beginPath();
        t.moveTo(cx - rx, cy);
        t.cubicTo(cx - rx, cy - oy, cx - ox, cy - ry, cx, cy - ry);
        t.cubicTo(cx + ox, cy - ry, cx + rx, cy - oy, cx + rx, cy);
        t.cubicTo(cx + rx, cy + oy, cx + ox, cy + ry, cx, cy + ry);
        t.cubicTo(cx - ox, cy + ry, cx - rx, cy + oy, cx - rx, cy);
        t.closePath();
        applyPaint(t, paint);
    }

    Path &Path::moveTo(double x, double y) { mSegs.push_back({Seg::Op::Move, {x, y}}); return *this; }
    Path &Path::lineTo(double x, double y) { mSegs.push_back({Seg::Op::Line, {x, y}}); return *this; }
    Path &Path::quadTo(double cx, double cy, double x, double y)
    {
        mSegs.push_back({Seg::Op::Quad, {cx, cy, x, y}});
        return *this;
    }
    Path &Path::cubicTo(double c1x, double c1y, double c2x, double c2y, double x, double y)
    {
        mSegs.push_back({Seg::Op::Cubic, {c1x, c1y, c2x, c2y, x, y}});
        return *this;
    }
    Path &Path::close() { mSegs.push_back({Seg::Op::Close}); return *this; }

    Path &Path::spline(const std::vector<Point> &pts)
    {
        if (pts.size() < 2)
            return *this;
        moveTo(pts[0].x, pts[0].y);
        const int n = (int)pts.size();
        for (int i = 0; i < n - 1; ++i)
        {
            const Point &p0 = pts[i > 0 ? i - 1 : 0];
            const Point &p1 = pts[i];
            const Point &p2 = pts[i + 1];
            const Point &p3 = pts[i + 2 < n ? i + 2 : n - 1];
            // Catmull-Rom -> cubic Bezier control points.
            double c1x = p1.x + (p2.x - p0.x) / 6.0, c1y = p1.y + (p2.y - p0.y) / 6.0;
            double c2x = p2.x - (p3.x - p1.x) / 6.0, c2y = p2.y - (p3.y - p1.y) / 6.0;
            cubicTo(c1x, c1y, c2x, c2y, p2.x, p2.y);
        }
        return *this;
    }

    void Path::onDraw(IRenderTarget &t) const
    {
        t.beginPath();
        for (const auto &s : mSegs)
        {
            switch (s.op)
            {
            case Seg::Op::Move:  t.moveTo(s.v[0], s.v[1]); break;
            case Seg::Op::Line:  t.lineTo(s.v[0], s.v[1]); break;
            case Seg::Op::Quad:  t.quadTo(s.v[0], s.v[1], s.v[2], s.v[3]); break;
            case Seg::Op::Cubic: t.cubicTo(s.v[0], s.v[1], s.v[2], s.v[3], s.v[4], s.v[5]); break;
            case Seg::Op::Close: t.closePath(); break;
            }
        }
        applyPaint(t, paint);
    }

    void Text::onDraw(IRenderTarget &t) const
    {
        t.setFill(color);
        t.drawText(text, position.x, position.y, sizePx);
    }
}
