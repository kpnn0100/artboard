#include "CairoTarget.h"

namespace artboard
{
    void CairoTarget::save()
    {
        cairo_save(mContext);
    }

    void CairoTarget::restore()
    {
        cairo_restore(mContext);
    }

    void CairoTarget::setTransform(const Transform &t)
    {
        cairo_matrix_t matrix;
        cairo_matrix_init(&matrix, t.a, t.b, t.c, t.d, t.e, t.f);
        cairo_set_matrix(mContext, &matrix);
    }

    void CairoTarget::clipRect(double x, double y, double w, double h)
    {
        cairo_new_path(mContext);
        cairo_rectangle(mContext, x, y, w, h);
        cairo_clip(mContext);
    }

    void CairoTarget::setFill(const Color &c)
    {
        cairo_set_source_rgba(mContext, c.r, c.g, c.b, c.a);
    }

    void CairoTarget::setRadialFill(double cx, double cy, double radius, const Color &inner, const Color &outer)
    {
        cairo_pattern_t *p = cairo_pattern_create_radial(cx, cy, 0.0, cx, cy, radius > 0.01 ? radius : 0.01);
        cairo_pattern_add_color_stop_rgba(p, 0.0, inner.r, inner.g, inner.b, inner.a);
        cairo_pattern_add_color_stop_rgba(p, 1.0, outer.r, outer.g, outer.b, outer.a);
        cairo_set_source(mContext, p);
        cairo_pattern_destroy(p);
    }

    void CairoTarget::setStroke(const Color &c, double width)
    {
        cairo_set_source_rgba(mContext, c.r, c.g, c.b, c.a);
        cairo_set_line_width(mContext, width);
    }

    void CairoTarget::beginPath()
    {
        cairo_new_path(mContext);
    }

    void CairoTarget::moveTo(double x, double y)
    {
        cairo_move_to(mContext, x, y);
    }

    void CairoTarget::lineTo(double x, double y)
    {
        cairo_line_to(mContext, x, y);
    }

    void CairoTarget::quadTo(double cx, double cy, double x, double y)
    {
        double x0 = 0.0;
        double y0 = 0.0;
        cairo_get_current_point(mContext, &x0, &y0);
        const double c1x = x0 + (2.0 / 3.0) * (cx - x0);
        const double c1y = y0 + (2.0 / 3.0) * (cy - y0);
        const double c2x = x + (2.0 / 3.0) * (cx - x);
        const double c2y = y + (2.0 / 3.0) * (cy - y);
        cairo_curve_to(mContext, c1x, c1y, c2x, c2y, x, y);
    }

    void CairoTarget::cubicTo(double c1x, double c1y, double c2x, double c2y, double x, double y)
    {
        cairo_curve_to(mContext, c1x, c1y, c2x, c2y, x, y);
    }

    void CairoTarget::closePath()
    {
        cairo_close_path(mContext);
    }

    // fill/stroke PRESERVE the current path (matching Canvas2D and the RecordingTarget op
    // stream); the path is cleared only by the next beginPath()/clipRect(). This lets a shape
    // fill then stroke the same path — how applyPaint draws a filledStroked (body + border).
    // Clearing the path here (cairo_new_path) would silently drop every border on native.
    void CairoTarget::fillPath()
    {
        cairo_fill_preserve(mContext);
    }

    void CairoTarget::strokePath()
    {
        cairo_stroke_preserve(mContext);
    }

    void CairoTarget::drawText(const std::string &text, double x, double y, double sizePx)
    {
        cairo_select_font_face(mContext, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(mContext, sizePx);
        cairo_move_to(mContext, x, y);
        cairo_show_text(mContext, text.c_str());
    }
}