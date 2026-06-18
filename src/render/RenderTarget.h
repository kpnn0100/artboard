/*
 *  Arstro Artboard — IRenderTarget: the drawing HAL.
 *
 *  The one seam between the platform-free core and any device. Minimal by design
 *  (ISP): graphics state, a path API, and text. Shapes emit themselves as paths +
 *  text, so an adapter implements only these primitives — never per-shape code.
 */
#pragma once
#include "../core/Geometry.h"
#include "../core/Color.h"
#include <string>

namespace artboard
{
    struct IRenderTarget
    {
        virtual ~IRenderTarget() = default;

        // ---- state stack ----
        virtual void save() = 0;
        virtual void restore() = 0;
        virtual void setTransform(const Transform &t) = 0;

        // ---- clip ----
        // Intersect the current clip region with the rectangle (in the current
        // transform space). Scoped by save()/restore(). A clip is a primitive
        // because no fill/stroke/path combination can restrict later drawing.
        virtual void clipRect(double x, double y, double w, double h) = 0;

        // ---- paint ----
        virtual void setFill(const Color &c) = 0;
        // Two-stop radial gradient fill (inner at centre -> outer at radius, current
        // transform space). The next fillPath() uses it. A smooth gradient is a primitive
        // because solid fills can only approximate it by stacking translucent shapes.
        virtual void setRadialFill(double cx, double cy, double radius, const Color &inner, const Color &outer) = 0;
        // Two-stop linear gradient fill along the axis (x0,y0)->(x1,y1) in the current
        // transform space (constant perpendicular to the axis). The next fillPath() uses it.
        // A primitive for the same reason as the radial fill: a smooth ramp cannot be built
        // from solid fills without banding. Gives depth/shading ramps.
        virtual void setLinearFill(double x0, double y0, double x1, double y1, const Color &start, const Color &end) = 0;
        virtual void setStroke(const Color &c, double width) = 0;

        // ---- path building ----
        virtual void beginPath() = 0;
        virtual void moveTo(double x, double y) = 0;
        virtual void lineTo(double x, double y) = 0;
        virtual void quadTo(double cx, double cy, double x, double y) = 0;
        virtual void cubicTo(double c1x, double c1y, double c2x, double c2y, double x, double y) = 0;
        virtual void closePath() = 0;

        // ---- paint the current path ----
        virtual void fillPath() = 0;
        virtual void strokePath() = 0;

        // ---- text (cannot be a path without a font; stays a primitive) ----
        virtual void drawText(const std::string &text, double x, double y, double sizePx) = 0;
    };
}
