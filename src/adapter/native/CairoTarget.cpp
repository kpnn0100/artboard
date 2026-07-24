#include "CairoTarget.h"
#include <algorithm>

#ifdef ARTBOARD_CAIRO_FT
#include <cairo/cairo-ft.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <map>
#include <string>
#endif

namespace artboard
{
#ifdef ARTBOARD_CAIRO_FT
    namespace
    {
        // Process-wide FreeType library + family -> cairo font face cache. Populated once
        // at startup via CairoTarget::registerFontFile; read (only) from drawText. The
        // FT_Face and cairo_font_face_t live for the process lifetime (never freed).
        FT_Library &ftLib()
        {
            static FT_Library lib = [] { FT_Library l = nullptr; FT_Init_FreeType(&l); return l; }();
            return lib;
        }
        std::map<std::string, cairo_font_face_t *> &ftFaces()
        {
            static std::map<std::string, cairo_font_face_t *> m;
            return m;
        }
    }

    void CairoTarget::registerFontFile(const std::string &family, const std::string &ttfPath)
    {
        if (family.empty() || ftFaces().count(family)) return;
        FT_Face face = nullptr;
        if (FT_New_Face(ftLib(), ttfPath.c_str(), 0, &face) != 0 || !face) return;
        ftFaces()[family] = cairo_ft_font_face_create_for_ft_face(face, 0);
    }
#endif

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

    // cairo_clip() (non-preserve) clears the current path as a side effect -- exactly the
    // "fresh path" postcondition clipPath() requires, mirroring clipRect(). Fill rule set
    // explicitly to nonzero (already Cairo's default) so the contract does not silently depend
    // on nothing else in the process ever changing it.
    void CairoTarget::clipPath()
    {
        cairo_set_fill_rule(mContext, CAIRO_FILL_RULE_WINDING);
        cairo_clip(mContext);
    }

    // cairo_push_group()/cairo_pop_group_to_source() already bracket an implicit
    // cairo_save()/cairo_restore() pair, so nothing else needs to be saved/restored here --
    // any transform/clip/paint change made between push and pop is undone by the pop, matching
    // this primitive's save()/restore()-like state-scope contract.
    void CairoTarget::pushLayer(double alpha)
    {
        mLayerAlphas.push_back(alpha);
        cairo_push_group(mContext);
    }

    void CairoTarget::popLayer()
    {
        if (mLayerAlphas.empty())
            return;  // unbalanced call: tolerate like releaseImage does for an unknown id
        const double alpha = mLayerAlphas.back();
        mLayerAlphas.pop_back();
        cairo_pop_group_to_source(mContext);
        cairo_paint_with_alpha(mContext, alpha);
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

    void CairoTarget::setLinearFill(double x0, double y0, double x1, double y1, const Color &start, const Color &end)
    {
        cairo_pattern_t *p = cairo_pattern_create_linear(x0, y0, x1, y1);
        cairo_pattern_add_color_stop_rgba(p, 0.0, start.r, start.g, start.b, start.a);
        cairo_pattern_add_color_stop_rgba(p, 1.0, end.r, end.g, end.b, end.a);
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

    namespace
    {
        // One UTF-8 codepoint's byte length from its leading byte (1 on a malformed/
        // continuation byte, so iteration always advances).
        size_t utf8SeqLen(unsigned char lead)
        {
            if ((lead & 0x80) == 0x00) return 1;
            if ((lead & 0xE0) == 0xC0) return 2;
            if ((lead & 0xF0) == 0xE0) return 3;
            if ((lead & 0xF8) == 0xF0) return 4;
            return 1;
        }
    }

    void CairoTarget::drawText(const std::string &text, double x, double y, double sizePx,
                                const std::string &fontFamily, double letterSpacingPx)
    {
#ifdef ARTBOARD_CAIRO_FT
        // Prefer an explicitly registered cairo-ft face (fontconfig-free hosts); fall
        // back to the toy API for any unregistered family.
        auto faceIt = ftFaces().find(fontFamily);
        if (faceIt != ftFaces().end() && faceIt->second)
            cairo_set_font_face(mContext, faceIt->second);
        else
            cairo_select_font_face(mContext, fontFamily.empty() ? "Sans" : fontFamily.c_str(),
                                    CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
#else
        cairo_select_font_face(mContext, fontFamily.empty() ? "Sans" : fontFamily.c_str(),
                                CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
#endif
        cairo_set_font_size(mContext, sizePx);

        if (letterSpacingPx == 0.0)
        {
            cairo_move_to(mContext, x, y);
            cairo_show_text(mContext, text.c_str());
            return;
        }

        // Extra tracking can't be expressed by one cairo_show_text call, so advance
        // glyph-by-glyph (UTF-8 codepoint aware) adding letterSpacingPx after each one.
        double cx = x;
        for (size_t i = 0; i < text.size();)
        {
            size_t len = utf8SeqLen(static_cast<unsigned char>(text[i]));
            len = std::min(len, text.size() - i);
            std::string glyph = text.substr(i, len);
            cairo_move_to(mContext, cx, y);
            cairo_show_text(mContext, glyph.c_str());
            cairo_text_extents_t extents;
            cairo_text_extents(mContext, glyph.c_str(), &extents);
            cx += extents.x_advance + letterSpacingPx;
            i += len;
        }
    }

    double CairoTarget::measureText(const std::string &text, double sizePx,
                                     const std::string &fontFamily, double letterSpacingPx) const
    {
        if (!mContext) return IRenderTarget::measureText(text, sizePx, fontFamily, letterSpacingPx);
#ifdef ARTBOARD_CAIRO_FT
        auto faceIt = ftFaces().find(fontFamily);
        if (faceIt != ftFaces().end() && faceIt->second)
            cairo_set_font_face(mContext, faceIt->second);
        else
            cairo_select_font_face(mContext, fontFamily.empty() ? "Sans" : fontFamily.c_str(),
                                    CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
#else
        cairo_select_font_face(mContext, fontFamily.empty() ? "Sans" : fontFamily.c_str(),
                                CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
#endif
        cairo_set_font_size(mContext, sizePx);
        if (letterSpacingPx == 0.0)
        {
            cairo_text_extents_t e;
            cairo_text_extents(mContext, text.c_str(), &e);
            return e.x_advance;  // matches drawText's single-show_text fast path
        }
        // Tracked text: sum per-codepoint advance + letterSpacing, mirroring drawText's pen.
        double cx = 0.0;
        for (size_t i = 0; i < text.size();)
        {
            size_t len = std::min(utf8SeqLen(static_cast<unsigned char>(text[i])), text.size() - i);
            std::string glyph = text.substr(i, len);
            cairo_text_extents_t e;
            cairo_text_extents(mContext, glyph.c_str(), &e);
            cx += e.x_advance + letterSpacingPx;
            i += len;
        }
        return cx;
    }

    CairoTarget::~CairoTarget()
    {
        for (auto &kv : mImages)
            if (kv.second.surface)
                cairo_surface_destroy(kv.second.surface);
    }

    // Convert HAL RGBA8-straight -> Cairo ARGB32, which on little-endian is BGRA in
    // memory and is premultiplied. Honors Cairo's required stride (may exceed w*4).
    void CairoTarget::buildEntry(ImageEntry &e, const uint8_t *rgba, int w, int h)
    {
        if (e.surface)
        {
            cairo_surface_destroy(e.surface);
            e.surface = nullptr;
        }
        const int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, w);
        e.w = w; e.h = h;
        e.data.assign((size_t)stride * h, 0);
        for (int y = 0; y < h; ++y)
        {
            const uint8_t *srow = rgba + (size_t)y * w * 4;
            uint8_t *drow = e.data.data() + (size_t)y * stride;
            for (int x = 0; x < w; ++x)
            {
                const uint8_t r = srow[x * 4 + 0];
                const uint8_t g = srow[x * 4 + 1];
                const uint8_t b = srow[x * 4 + 2];
                const uint8_t a = srow[x * 4 + 3];
                drow[x * 4 + 0] = (uint8_t)((b * a + 127) / 255);  // B
                drow[x * 4 + 1] = (uint8_t)((g * a + 127) / 255);  // G
                drow[x * 4 + 2] = (uint8_t)((r * a + 127) / 255);  // R
                drow[x * 4 + 3] = a;                                // A
            }
        }
        e.surface = cairo_image_surface_create_for_data(
            e.data.data(), CAIRO_FORMAT_ARGB32, w, h, stride);
        cairo_surface_mark_dirty(e.surface);
    }

    int CairoTarget::registerImage(const uint8_t *rgba, int w, int h)
    {
        if (!rgba || w <= 0 || h <= 0)
            return 0;
        const int id = mNextImageId++;
        buildEntry(mImages[id], rgba, w, h);
        return id;
    }

    void CairoTarget::updateImage(int id, const uint8_t *rgba, int w, int h)
    {
        if (!rgba || w <= 0 || h <= 0)
            return;
        auto it = mImages.find(id);
        if (it == mImages.end())
            return;
        buildEntry(it->second, rgba, w, h);
    }

    void CairoTarget::drawImage(int id, const Rect &dst)
    {
        auto it = mImages.find(id);
        if (it == mImages.end() || !it->second.surface || it->second.w <= 0 || it->second.h <= 0)
            return;
        const ImageEntry &e = it->second;
        cairo_save(mContext);
        cairo_translate(mContext, dst.x, dst.y);
        cairo_scale(mContext, dst.w / e.w, dst.h / e.h);
        cairo_set_source_surface(mContext, e.surface, 0, 0);
        cairo_pattern_set_filter(cairo_get_source(mContext), CAIRO_FILTER_GOOD);
        cairo_paint(mContext);
        cairo_restore(mContext);
    }

    void CairoTarget::releaseImage(int id)
    {
        auto it = mImages.find(id);
        if (it == mImages.end())
            return;
        if (it->second.surface)
            cairo_surface_destroy(it->second.surface);
        mImages.erase(it);
    }
}