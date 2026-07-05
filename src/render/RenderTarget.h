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
#include <cstdint>

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
        // fontFamily selects a family name the adapter's text stack resolves (Fontconfig on
        // native, the CSS font stack on web); empty keeps each adapter's generic default. A
        // distinct static weight (Medium, SemiBold, ...) is selected by passing that weight's
        // own family name -- not a separate weight enum -- since real static weights are
        // distinct font files/families, and Cairo's weight enum only has two values anyway.
        // letterSpacingPx adds extra advance between glyphs (0 = normal tracking); an adapter
        // that can't set it natively falls back to manual glyph-by-glyph advance.
        virtual void drawText(const std::string &text, double x, double y, double sizePx,
                               const std::string &fontFamily = "", double letterSpacingPx = 0.0) = 0;

        // ---- raster images (handle/registration model) ----
        // A raster image is a primitive: no path/fill/text combination reproduces a
        // photograph's per-pixel colour. The handle model uploads the pixels ONCE
        // (registerImage) and re-uploads only on change (updateImage), so a large
        // photo is not re-sent every frame; drawImage is then a cheap blit into the
        // destination rect (current transform space).
        //
        // Pixel format for register/update is fixed: tightly-packed RGBA8, 4 bytes
        // per pixel, row-major top-to-bottom, stride = w*4, STRAIGHT (non-pre-
        // multiplied) alpha, sRGB. Each adapter converts to its native expectation.
        virtual int  registerImage(const uint8_t *rgba, int w, int h) = 0; // -> id>0 (0 = failed)
        virtual void updateImage(int id, const uint8_t *rgba, int w, int h) = 0;
        virtual void drawImage(int id, const Rect &dst) = 0;               // no-op if id unknown
        virtual void releaseImage(int id) = 0;
    };
}
