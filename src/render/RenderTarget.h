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

        // ---- paint ----
        virtual void setFill(const Color &c) = 0;
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
