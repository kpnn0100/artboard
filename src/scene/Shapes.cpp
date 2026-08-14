#include "Shapes.h"
#include <array>
#include <cmath>
#include <vector>

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

    Path roundedRectPath(const Rect &rect, double cornerRadius)
    {
        const double x = rect.x, y = rect.y, w = rect.w, h = rect.h;
        Path p;
        if (cornerRadius > 0.0)
        {
            // Clamp radius to half the smaller side.
            double r = cornerRadius;
            const double half = (w < h ? w : h) * 0.5;
            if (r > half) r = half;
            // Each corner is a quarter CIRCLE (FR-45), so it matches the circles drawn
            // beside it. A quadratic with its control point at the box corner — the obvious
            // shortcut — bulges ~6% outward at the middle of the corner and reads as squarer
            // than the radius says. The cubic kappa offset below holds the outline within
            // 0.1% of a true arc, and is the same constant ellipsePath uses.
            const double k = r * 0.5522847498307936;
            p.moveTo(x + r, y);
            p.lineTo(x + w - r, y);
            p.cubicTo(x + w - r + k, y, x + w, y + r - k, x + w, y + r);          // top-right
            p.lineTo(x + w, y + h - r);
            p.cubicTo(x + w, y + h - r + k, x + w - r + k, y + h, x + w - r, y + h);  // bottom-right
            p.lineTo(x + r, y + h);
            p.cubicTo(x + r - k, y + h, x, y + h - r + k, x, y + h - r);          // bottom-left
            p.lineTo(x, y + r);
            p.cubicTo(x, y + r - k, x + r - k, y, x + r, y);                      // top-left
            p.close();
        }
        else
        {
            p.moveTo(x, y);
            p.lineTo(x + w, y);
            p.lineTo(x + w, y + h);
            p.lineTo(x, y + h);
            p.close();
        }
        return p;
    }

    bool Arc::active() const
    {
        return innerRatio > 0.0 || std::fabs(sweep) < 6.283185307179586 - 1e-9 || start != 0.0;
    }

    namespace
    {
        constexpr double kTau = 6.283185307179586;

        Point onEllipse(double cx, double cy, double rx, double ry, double a)
        {
            return Point{cx + rx * std::cos(a), cy + ry * std::sin(a)};
        }

        /*  Append the elliptical arc from `a0` to `a0 + sweep` to `p`, as cubic segments of at
         *  most 90 degrees each. The control-point scale is the standard 4/3*tan(delta/4),
         *  which is what makes the approximation accurate at any radius — so no new HAL
         *  primitive is needed to draw a true arc.
         */
        void appendArc(Path &p, double cx, double cy, double rx, double ry, double a0, double sweep)
        {
            const int steps = std::max(1, (int)std::ceil(std::fabs(sweep) / (kTau / 4.0)));
            const double delta = sweep / steps;
            const double k = 4.0 / 3.0 * std::tan(delta / 4.0);
            double a = a0;
            for (int i = 0; i < steps; ++i)
            {
                const double a1 = a + delta;
                const Point p0 = onEllipse(cx, cy, rx, ry, a);
                const Point p1 = onEllipse(cx, cy, rx, ry, a1);
                // Tangents of the parametric ellipse at each end.
                const Point t0{-rx * std::sin(a), ry * std::cos(a)};
                const Point t1{-rx * std::sin(a1), ry * std::cos(a1)};
                p.cubicTo(p0.x + k * t0.x, p0.y + k * t0.y,
                          p1.x - k * t1.x, p1.y - k * t1.y, p1.x, p1.y);
                a = a1;
            }
        }
    }

    Path ellipseArcPath(double cx, double cy, double rx, double ry,
                        double startRad, double sweepRad, double innerRatio)
    {
        Path p;
        if (sweepRad == 0.0 || rx <= 0.0 || ry <= 0.0)
            return p;   // nothing to draw

        const double ratio = innerRatio < 0.0 ? 0.0 : (innerRatio > 1.0 ? 1.0 : innerRatio);
        const bool full = std::fabs(sweepRad) >= kTau - 1e-9;
        const double sweep = full ? (sweepRad < 0.0 ? -kTau : kTau) : sweepRad;

        if (full)
        {
            // The whole disk: the plain ellipse, or an annulus whose inner contour is wound
            // the OTHER way so nonzero winding leaves the hole empty.
            const Point s = onEllipse(cx, cy, rx, ry, startRad);
            p.moveTo(s.x, s.y);
            appendArc(p, cx, cy, rx, ry, startRad, sweep);
            p.close();
            if (ratio > 0.0)
            {
                const double irx = rx * ratio, iry = ry * ratio;
                const Point is = onEllipse(cx, cy, irx, iry, startRad + sweep);
                p.moveTo(is.x, is.y);
                appendArc(p, cx, cy, irx, iry, startRad + sweep, -sweep);
                p.close();
            }
            return p;
        }

        if (ratio <= 0.0)
        {
            // A pie: centre, out to the arc's start, round, and back to the centre. The two
            // straight edges are the rays that make a pac-man's mouth.
            p.moveTo(cx, cy);
            const Point s = onEllipse(cx, cy, rx, ry, startRad);
            p.lineTo(s.x, s.y);
            appendArc(p, cx, cy, rx, ry, startRad, sweep);
            p.close();
            return p;
        }

        // A ring segment: out along the outer arc, across, back along the inner one.
        const double irx = rx * ratio, iry = ry * ratio;
        const Point outerStart = onEllipse(cx, cy, rx, ry, startRad);
        p.moveTo(outerStart.x, outerStart.y);
        appendArc(p, cx, cy, rx, ry, startRad, sweep);
        const Point innerEnd = onEllipse(cx, cy, irx, iry, startRad + sweep);
        p.lineTo(innerEnd.x, innerEnd.y);
        appendArc(p, cx, cy, irx, iry, startRad + sweep, -sweep);
        p.close();
        return p;
    }

    Path ellipsePath(double cx, double cy, double rx, double ry)
    {
        const double k = 0.5522847498307936;   // cubic bezier circle constant
        const double ox = rx * k, oy = ry * k;
        Path p;
        p.moveTo(cx - rx, cy);
        p.cubicTo(cx - rx, cy - oy, cx - ox, cy - ry, cx, cy - ry);
        p.cubicTo(cx + ox, cy - ry, cx + rx, cy - oy, cx + rx, cy);
        p.cubicTo(cx + rx, cy + oy, cx + ox, cy + ry, cx, cy + ry);
        p.cubicTo(cx - ox, cy + ry, cx - rx, cy + oy, cx - rx, cy);
        p.close();
        return p;
    }

    // Both drawing helpers go through the builders, so the outline of a circle or a rounded
    // rect has exactly ONE definition — the one a trim (FR-42) also operates on.
    void drawRoundedRect(IRenderTarget &t, const Rect &rect, double cornerRadius, const Paint &paint)
    {
        Path p = roundedRectPath(rect, cornerRadius);
        p.paint = paint;
        p.emit(t);
    }

    void drawCircle(IRenderTarget &t, double cx, double cy, double r, const Paint &paint)
    {
        Path p = ellipsePath(cx, cy, r, r);
        p.paint = paint;
        p.emit(t);
    }

    void drawShadow(IRenderTarget &t, const Rect &rect, double cornerRadius, const Color &color,
                     double blurPx, double offsetX, double offsetY)
    {
        if (blurPx <= 0.0)
            return;

        const double sx = rect.x + offsetX, sy = rect.y + offsetY, sw = rect.w, sh = rect.h;
        double r = cornerRadius;
        const double half = (sw < sh ? sw : sh) * 0.5;
        if (r > half) r = half;
        if (r < 0.0) r = 0.0;
        const double b = blurPx;

        const Color peak = color;
        const Color zero{color.r, color.g, color.b, 0.0};

        // Four straight-edge strips, skipped if their span would be non-positive (a very small
        // rect where 2*r already meets or exceeds a side).
        if (sx + sw - r > sx + r)
        {
            // top
            t.setLinearFill(sx + r, sy, sx + r, sy - b, peak, zero);
            t.beginPath();
            t.moveTo(sx + r, sy);       t.lineTo(sx + sw - r, sy);
            t.lineTo(sx + sw - r, sy - b); t.lineTo(sx + r, sy - b);
            t.closePath();
            t.fillPath();
            // bottom
            t.setLinearFill(sx + r, sy + sh, sx + r, sy + sh + b, peak, zero);
            t.beginPath();
            t.moveTo(sx + r, sy + sh);       t.lineTo(sx + sw - r, sy + sh);
            t.lineTo(sx + sw - r, sy + sh + b); t.lineTo(sx + r, sy + sh + b);
            t.closePath();
            t.fillPath();
        }
        if (sy + sh - r > sy + r)
        {
            // left
            t.setLinearFill(sx, sy + r, sx - b, sy + r, peak, zero);
            t.beginPath();
            t.moveTo(sx, sy + r);       t.lineTo(sx, sy + sh - r);
            t.lineTo(sx - b, sy + sh - r); t.lineTo(sx - b, sy + r);
            t.closePath();
            t.fillPath();
            // right
            t.setLinearFill(sx + sw, sy + r, sx + sw + b, sy + r, peak, zero);
            t.beginPath();
            t.moveTo(sx + sw, sy + r);       t.lineTo(sx + sw, sy + sh - r);
            t.lineTo(sx + sw + b, sy + sh - r); t.lineTo(sx + sw + b, sy + r);
            t.closePath();
            t.fillPath();
        }

        // Four corner wedges: a "kite" from the arc centre C out to radius r+b (approximated
        // with one quadTo, mirroring drawRoundedRect's own corner curve), filled with a radial
        // gradient centred at C. sxs/sys pick which of the object's four corners this is.
        const double corners[4][2] = {{-1, -1}, {1, -1}, {1, 1}, {-1, 1}}; // (sxs, sys)
        for (const auto &c : corners)
        {
            const double sxs = c[0], sys = c[1];
            const double cx = sx + (sxs < 0 ? r : sw - r);
            const double cy = sy + (sys < 0 ? r : sh - r);
            const double ph_x = cx + sxs * (r + b), ph_y = cy;
            const double pv_x = cx, pv_y = cy + sys * (r + b);
            const double ctrl_x = cx + sxs * (r + b), ctrl_y = cy + sys * (r + b);

            t.setRadialFill(cx, cy, r + b, peak, zero);
            t.beginPath();
            t.moveTo(cx, cy);
            t.lineTo(ph_x, ph_y);
            t.quadTo(ctrl_x, ctrl_y, pv_x, pv_y);
            t.lineTo(cx, cy);
            t.closePath();
            t.fillPath();
        }
    }

    void drawElevation(IRenderTarget &t, const Rect &rect, double cornerRadius, double elevationDp)
    {
        // Ambient layer first (softer/wider), then key layer on top (tighter/darker) --
        // matches Material's convention of compositing key over ambient.
        drawShadow(t, rect, cornerRadius, Color::rgba(0, 0, 0, (int)(0.15 * 255)),
                   elevationDp * 2.0, 0.0, elevationDp * 0.25);
        drawShadow(t, rect, cornerRadius, Color::rgba(0, 0, 0, (int)(0.30 * 255)),
                   elevationDp * 1.0, 0.0, elevationDp * 0.5);
    }

    void Rectangle::onDraw(IRenderTarget &t) const
    {
        drawRoundedRect(t, rect, cornerRadius, paint);
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

    // ───────────────────────── trim (FR-42) ─────────────────────────
    namespace
    {
        /** Samples per curve used to measure arc length and to locate a split parameter.
         *  32 keeps a quarter-circle's length within a fraction of a pixel at UI sizes, which
         *  is well below what the eye (or a RecordingTarget assertion) can see. */
        constexpr int kArcSamples = 32;

        Point lerpPoint(const Point &a, const Point &b, double t)
        {
            return Point{a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
        }
        Point cubicAt(const Point &p0, const Point &c1, const Point &c2, const Point &p1, double t)
        {
            const Point a = lerpPoint(p0, c1, t), b = lerpPoint(c1, c2, t), c = lerpPoint(c2, p1, t);
            const Point d = lerpPoint(a, b, t), e = lerpPoint(b, c, t);
            return lerpPoint(d, e, t);
        }
        double dist(const Point &a, const Point &b)
        {
            const double dx = b.x - a.x, dy = b.y - a.y;
            return std::sqrt(dx * dx + dy * dy);
        }

    }

    /** One drawable piece of a path, already reduced to a line or a cubic. Quadratics are
     *  raised to cubics so the splitter has exactly two cases instead of three. */
    struct Path::Piece
    {
            bool isCubic = false;
            Point p0, c1, c2, p1;
            double length = 0.0;
            /** Cumulative length at each sample, for locating a split parameter. */
            std::vector<double> marks;

            void measure()
            {
                if (!isCubic)
                {
                    length = dist(p0, p1);
                    return;
                }
                marks.assign(kArcSamples + 1, 0.0);
                Point prev = p0;
                double acc = 0.0;
                for (int i = 1; i <= kArcSamples; ++i)
                {
                    const Point cur = cubicAt(p0, c1, c2, p1, (double)i / kArcSamples);
                    acc += dist(prev, cur);
                    marks[(size_t)i] = acc;
                    prev = cur;
                }
                length = acc;
            }

            /** The curve parameter at `d` along this piece (0..length). */
            double paramAt(double d) const
            {
                if (!isCubic || length <= 0.0)
                    return length <= 0.0 ? 0.0 : d / length;
                for (int i = 1; i <= kArcSamples; ++i)
                    if (marks[(size_t)i] >= d)
                    {
                        const double lo = marks[(size_t)(i - 1)], hi = marks[(size_t)i];
                        const double f = hi > lo ? (d - lo) / (hi - lo) : 0.0;
                        return ((double)(i - 1) + f) / kArcSamples;
                    }
                return 1.0;
            }

            /** The sub-piece between two curve parameters, by de Casteljau — so a trimmed
             *  curve follows the ORIGINAL curve, not a polyline through it. */
            Piece slice(double t0, double t1) const
            {
                Piece out;
                out.isCubic = isCubic;
                if (!isCubic)
                {
                    out.p0 = lerpPoint(p0, p1, t0);
                    out.p1 = lerpPoint(p0, p1, t1);
                    return out;
                }
                // Split at t1 first (keep the head), then re-split that at the rescaled t0.
                const Point a = lerpPoint(p0, c1, t1), b = lerpPoint(c1, c2, t1), c = lerpPoint(c2, p1, t1);
                const Point d = lerpPoint(a, b, t1), e = lerpPoint(b, c, t1);
                const Point end = lerpPoint(d, e, t1);
                const double t = t1 > 0.0 ? t0 / t1 : 0.0;
                const Point a2 = lerpPoint(p0, a, t), b2 = lerpPoint(a, d, t), c2b = lerpPoint(d, end, t);
                const Point d2 = lerpPoint(a2, b2, t), e2 = lerpPoint(b2, c2b, t);
                out.p0 = lerpPoint(d2, e2, t);
                out.c1 = e2;
                out.c2 = c2b;
                out.p1 = end;
                return out;
            }
    };

    Path &Path::clear()
    {
        mSegs.clear();
        return *this;
    }

    /*  Reduce this path to measured pieces. A `close` becomes the line back to the current
     *  subpath's start, which is what makes a closed shape trim as one continuous run rather
     *  than stopping at the seam.
     */
    std::vector<Path::Piece> Path::pieces() const
    {
        std::vector<Path::Piece> out;
        Point cur{0, 0}, sub{0, 0};
        bool have = false;
        auto push = [&](Path::Piece p) {
            p.measure();
            if (p.length > 0.0)
                out.push_back(std::move(p));
        };
        for (const auto &s : mSegs)
        {
            switch (s.op)
            {
            case Seg::Op::Move:
                cur = sub = Point{s.v[0], s.v[1]};
                have = true;
                break;
            case Seg::Op::Line:
            {
                if (!have) { cur = sub = Point{s.v[0], s.v[1]}; have = true; break; }
                Path::Piece p;
                p.p0 = cur;
                p.p1 = Point{s.v[0], s.v[1]};
                push(p);
                cur = p.p1;
                break;
            }
            case Seg::Op::Quad:
            {
                if (!have) break;
                // Raise the quadratic to a cubic so the splitter has two cases, not three.
                const Point q{s.v[0], s.v[1]}, end{s.v[2], s.v[3]};
                Path::Piece p;
                p.isCubic = true;
                p.p0 = cur;
                p.c1 = Point{cur.x + 2.0 / 3.0 * (q.x - cur.x), cur.y + 2.0 / 3.0 * (q.y - cur.y)};
                p.c2 = Point{end.x + 2.0 / 3.0 * (q.x - end.x), end.y + 2.0 / 3.0 * (q.y - end.y)};
                p.p1 = end;
                push(p);
                cur = end;
                break;
            }
            case Seg::Op::Cubic:
            {
                if (!have) break;
                Path::Piece p;
                p.isCubic = true;
                p.p0 = cur;
                p.c1 = Point{s.v[0], s.v[1]};
                p.c2 = Point{s.v[2], s.v[3]};
                p.p1 = Point{s.v[4], s.v[5]};
                push(p);
                cur = p.p1;
                break;
            }
            case Seg::Op::Close:
            {
                if (!have) break;
                Path::Piece p;
                p.p0 = cur;
                p.p1 = sub;
                push(p);
                cur = sub;
                break;
            }
            }
        }
        return out;
    }

    double Path::length() const
    {
        double total = 0.0;
        for (const auto &p : pieces())
            total += p.length;
        return total;
    }

    Path Path::trimmed(double start, double end, double offset) const
    {
        Path out;
        out.paint = paint;
        out.transform = transform;
        out.visible = visible;

        const std::vector<Path::Piece> ps = pieces();
        double total = 0.0;
        for (const auto &p : ps)
            total += p.length;
        if (ps.empty() || total <= 0.0)
            return out;

        double a = start + offset, b = end + offset;
        if (b - a >= 1.0)
            a = 0.0, b = 1.0;                       // a full turn (or more) is the whole path
        else
        {
            const double wrap = std::floor(a);      // bring the pair into [0,1) together
            a -= wrap;
            b -= wrap;
        }
        if (b <= a)
            return out;                             // empty range draws nothing

        // A range that runs past the end wraps around: emit the tail, then the head. This is
        // what lets a spinner's arc cross the path's seam instead of snapping at it.
        if (b > 1.0)
        {
            Path first = trimmedRange(ps, total, a, 1.0);
            Path second = trimmedRange(ps, total, 0.0, b - 1.0);
            out.mSegs = std::move(first.mSegs);
            for (const auto &s : second.mSegs)
                out.mSegs.push_back(s);
            return out;
        }
        Path only = trimmedRange(ps, total, a, b);
        out.mSegs = std::move(only.mSegs);
        return out;
    }

    /** The sub-path between two fractions, both already inside [0,1]. */
    Path Path::trimmedRange(const std::vector<Path::Piece> &ps, double total, double a, double b)
    {
        Path out;
        const double from = a * total, to = b * total;
        double walked = 0.0;
        bool started = false;
        for (const auto &p : ps)
        {
            const double pieceStart = walked, pieceEnd = walked + p.length;
            walked = pieceEnd;
            if (pieceEnd <= from || pieceStart >= to)
                continue;                            // entirely outside the range

            const double localFrom = std::max(0.0, from - pieceStart);
            const double localTo = std::min(p.length, to - pieceStart);
            const double t0 = p.paramAt(localFrom), t1 = p.paramAt(localTo);
            const Path::Piece cut = p.slice(t0, t1);

            if (!started)
            {
                out.moveTo(cut.p0.x, cut.p0.y);
                started = true;
            }
            if (cut.isCubic)
                out.cubicTo(cut.c1.x, cut.c1.y, cut.c2.x, cut.c2.y, cut.p1.x, cut.p1.y);
            else
                out.lineTo(cut.p1.x, cut.p1.y);
        }
        return out;   // deliberately OPEN: a trim is a cut, so close() is not re-applied
    }

    void Path::onDraw(IRenderTarget &t) const { emit(t); }

    void Path::emit(IRenderTarget &t) const
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
        t.drawText(text, position.x, position.y, sizePx, fontFamily, letterSpacingPx);
    }
}
