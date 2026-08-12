/*
 *  Arstro Artboard — shared interaction (hover) treatment (FR-24).
 *
 *  One source of truth for how a clickable control looks under the pointer, so
 *  hover reads identically framework-wide and works for ANY theme without adding
 *  per-control hover fields (consistency lock, design taste §2A). Controls scale
 *  the treatment by their animated `hoverAmount()` in [0,1] so it never pops.
 *  Platform-free: pure colour maths, no HAL.
 */
#pragma once
#include "Theme.h"

namespace artboard
{
    namespace interaction
    {
        constexpr double kHoverMs = 120.0;       // hover fade in/out (short, per §2A)
        constexpr double kHoverFillLift = 0.14;  // brighten fill toward white at full hover
        constexpr double kHoverStrokeLift = 0.5; // pull border toward the emphasis colour
        constexpr double kDisabledFade = 0.55;   // alpha removed at full disable (FR-40)
    }

    /** Linear interpolate two colours componentwise (t in [0,1]). */
    inline Color lerpColor(const Color &a, const Color &b, double t)
    {
        return Color{a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t,
                     a.b + (b.b - a.b) * t, a.a + (b.a - a.a) * t};
    }

    /** Brighten toward white by `amt` in [0,1], preserving alpha. */
    inline Color brighten(const Color &c, double amt)
    {
        return Color{c.r + (1.0 - c.r) * amt, c.g + (1.0 - c.g) * amt,
                     c.b + (1.0 - c.b) * amt, c.a};
    }

    /** Interpolate a whole Paint (fill/stroke/width) from a→b by t. */
    inline Paint lerpPaint(const Paint &a, const Paint &b, double t)
    {
        Paint p = a;
        if (a.hasFill || b.hasFill)
        {
            p.hasFill = true;
            p.fill = lerpColor(a.fill, b.fill, t);
        }
        if (a.hasStroke || b.hasStroke)
        {
            p.hasStroke = true;
            p.stroke = lerpColor(a.stroke, b.stroke, t);
            p.strokeWidth = a.strokeWidth + (b.strokeWidth - a.strokeWidth) * t;
        }
        return p;
    }

    /** Interpolate a whole BoxStyle (paint + corner radius) from a→b by t. */
    inline BoxStyle lerpBox(const BoxStyle &a, const BoxStyle &b, double t)
    {
        return BoxStyle{lerpPaint(a.paint, b.paint, t),
                        a.cornerRadius + (b.cornerRadius - a.cornerRadius) * t};
    }

    /** Standard disabled treatment (FR-40): drop `t` of the colour's alpha, so a disabled
     *  control reads as unavailable on any surface without inventing a per-control grey. */
    inline Color dimColor(const Color &c, double t)
    {
        Color out = c;
        out.a *= 1.0 - interaction::kDisabledFade * t;
        return out;
    }

    inline Paint dimPaint(const Paint &p, double t)
    {
        Paint out = p;
        if (out.hasFill) out.fill = dimColor(out.fill, t);
        if (out.hasStroke) out.stroke = dimColor(out.stroke, t);
        return out;
    }

    inline BoxStyle dimBox(const BoxStyle &b, double t)
    {
        return BoxStyle{dimPaint(b.paint, t), b.cornerRadius};
    }

    /** Standard hover appearance for a box, scaled by `t` (the control's hoverAmount):
     *  brighten the fill and pull the border toward `emphasis` (the control's accent). */
    inline BoxStyle hoverBox(const BoxStyle &base, const Color &emphasis, double t)
    {
        BoxStyle s = base;
        s.paint.fill = brighten(base.paint.fill, interaction::kHoverFillLift * t);
        if (base.paint.hasStroke)
            s.paint.stroke = lerpColor(base.paint.stroke, emphasis, interaction::kHoverStrokeLift * t);
        return s;
    }
}
