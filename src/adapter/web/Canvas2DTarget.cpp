#include "Canvas2DTarget.h"
#include <emscripten.h>
#include <cstdint>

// Each primitive maps to one Canvas2D call on the active context (window.__abctx,
// set by the page before rendering). Colors become CSS rgba() strings.
EM_JS(void, ab_save, (), { window.__abctx.save(); });
EM_JS(void, ab_restore, (), { window.__abctx.restore(); });
EM_JS(void, ab_xform, (double a, double b, double c, double d, double e, double f),
      { window.__abctx.setTransform(a, b, c, d, e, f); });
EM_JS(void, ab_clip, (double x, double y, double w, double h),
      { var c = window.__abctx; c.beginPath(); c.rect(x, y, w, h); c.clip(); });
// clip() alone does NOT clear Canvas2D's current path (unlike Cairo's cairo_clip(), which does);
// the trailing beginPath() matches that postcondition so both adapters leave identical state.
EM_JS(void, ab_clipPath, (), { var c = window.__abctx; c.clip(); c.beginPath(); });

// Opacity layer (group compositing): Canvas2D has no native push-group call, so each layer is
// an offscreen <canvas> the same size as the current target. Every ab_* drawing function above
// already reads window.__abctx fresh on each call, so redirecting THAT ONE BINDING for the
// layer's lifetime redirects every primitive automatically -- no other function needs to change.
EM_JS(void, ab_pushLayer, (double alpha), {
    if (!window.__abLayerStack) window.__abLayerStack = [];
    var prev = window.__abctx;
    var cv = document.createElement('canvas');
    cv.width = prev.canvas.width;
    cv.height = prev.canvas.height;
    var ctx = cv.getContext('2d');
    // Copy transform + paint state so content drawn in the layer positions/paints exactly as it
    // would on the destination (Cairo's push_group does this automatically via cairo_save()).
    ctx.setTransform(prev.getTransform());
    ctx.fillStyle = prev.fillStyle;
    ctx.strokeStyle = prev.strokeStyle;
    ctx.lineWidth = prev.lineWidth;
    ctx.font = prev.font;
    window.__abLayerStack.push({ ctx: prev, alpha: alpha });
    window.__abctx = ctx;
});
EM_JS(void, ab_popLayer, (), {
    if (!window.__abLayerStack || window.__abLayerStack.length === 0) return;
    var layerCanvas = window.__abctx.canvas;
    var top = window.__abLayerStack.pop();
    var dest = top.ctx;
    window.__abctx = dest;
    // The layer canvas's pixels are already positioned in device space (its transform matched
    // the destination's at push time), so composite with an identity transform -- applying the
    // destination's current transform again would transform it a second time. dest's own active
    // clip (untouched throughout, since all layer drawing happened on the separate offscreen
    // context) still constrains this drawImage, matching Cairo's resumed pre-push clip.
    dest.save();
    dest.setTransform(1, 0, 0, 1, 0, 0);
    dest.globalAlpha = top.alpha;
    dest.drawImage(layerCanvas, 0, 0);
    dest.restore();
});
EM_JS(void, ab_fillStyle, (double r, double g, double b, double a),
      { window.__abctx.fillStyle = 'rgba(' + (r * 255 | 0) + ',' + (g * 255 | 0) + ',' + (b * 255 | 0) + ',' + a + ')'; });
EM_JS(void, ab_radialFill, (double cx, double cy, double rad, double ir, double ig, double ib, double ia, double orr, double og, double ob, double oa),
      { var c = window.__abctx; var g = c.createRadialGradient(cx, cy, 0, cx, cy, rad > 0.01 ? rad : 0.01);
        g.addColorStop(0, 'rgba(' + (ir * 255 | 0) + ',' + (ig * 255 | 0) + ',' + (ib * 255 | 0) + ',' + ia + ')');
        g.addColorStop(1, 'rgba(' + (orr * 255 | 0) + ',' + (og * 255 | 0) + ',' + (ob * 255 | 0) + ',' + oa + ')');
        c.fillStyle = g; });
EM_JS(void, ab_linearFill, (double x0, double y0, double x1, double y1, double sr, double sg, double sb, double sa, double er, double eg, double eb, double ea),
      { var c = window.__abctx; var g = c.createLinearGradient(x0, y0, x1, y1);
        g.addColorStop(0, 'rgba(' + (sr * 255 | 0) + ',' + (sg * 255 | 0) + ',' + (sb * 255 | 0) + ',' + sa + ')');
        g.addColorStop(1, 'rgba(' + (er * 255 | 0) + ',' + (eg * 255 | 0) + ',' + (eb * 255 | 0) + ',' + ea + ')');
        c.fillStyle = g; });
EM_JS(void, ab_strokeStyle, (double r, double g, double b, double a, double w),
      { var c = window.__abctx; c.strokeStyle = 'rgba(' + (r * 255 | 0) + ',' + (g * 255 | 0) + ',' + (b * 255 | 0) + ',' + a + ')'; c.lineWidth = w; });
EM_JS(void, ab_begin, (), { window.__abctx.beginPath(); });
EM_JS(void, ab_moveTo, (double x, double y), { window.__abctx.moveTo(x, y); });
EM_JS(void, ab_lineTo, (double x, double y), { window.__abctx.lineTo(x, y); });
EM_JS(void, ab_quad, (double cx, double cy, double x, double y), { window.__abctx.quadraticCurveTo(cx, cy, x, y); });
EM_JS(void, ab_cubic, (double a, double b, double c, double d, double x, double y), { window.__abctx.bezierCurveTo(a, b, c, d, x, y); });
EM_JS(void, ab_close, (), { window.__abctx.closePath(); });
EM_JS(void, ab_fill, (), { window.__abctx.fill(); });
EM_JS(void, ab_stroke, (), { window.__abctx.stroke(); });
EM_JS(void, ab_text, (const char *s, double x, double y, double size, const char *family, double spacing),
      { var c = window.__abctx; var fam = UTF8ToString(family);
        c.font = size + 'px ' + (fam ? ('"' + fam + '", sans-serif') : 'sans-serif');
        if ('letterSpacing' in c) c.letterSpacing = spacing + 'px';
        c.fillText(UTF8ToString(s), x, y); });

EM_JS(double, ab_measure_text, (const char *s, double size, const char *family, double spacing),
      { var c = window.__abctx; var fam = UTF8ToString(family); var str = UTF8ToString(s);
        c.font = size + 'px ' + (fam ? ('"' + fam + '", sans-serif') : 'sans-serif');
        var hasLS = ('letterSpacing' in c);
        if (hasLS) c.letterSpacing = spacing + 'px';
        var w = c.measureText(str).width;
        if (!hasLS && str.length > 1) w += (str.length - 1) * spacing;  // manual tracking fallback
        return w; });

// Raster images: each id is an offscreen <canvas> in window.__abimg.map. Canvas2D
// ImageData is straight RGBA8, top-down — exactly the HAL format (no conversion).
EM_JS(int, ab_registerImage, (uintptr_t ptr, int w, int h), {
    if (!window.__abimg) window.__abimg = { next: 1, map: {} };
    var id = window.__abimg.next++;
    var cv = document.createElement('canvas'); cv.width = w; cv.height = h;
    var ctx = cv.getContext('2d');
    var img = ctx.createImageData(w, h);
    img.data.set(HEAPU8.subarray(ptr, ptr + w * h * 4));
    ctx.putImageData(img, 0, 0);
    window.__abimg.map[id] = cv;
    return id;
});
EM_JS(void, ab_updateImage, (int id, uintptr_t ptr, int w, int h), {
    if (!window.__abimg || !window.__abimg.map[id]) return;
    var cv = window.__abimg.map[id]; cv.width = w; cv.height = h;
    var ctx = cv.getContext('2d');
    var img = ctx.createImageData(w, h);
    img.data.set(HEAPU8.subarray(ptr, ptr + w * h * 4));
    ctx.putImageData(img, 0, 0);
});
EM_JS(void, ab_drawImage, (int id, double x, double y, double w, double h), {
    if (!window.__abimg || !window.__abimg.map[id]) return;
    window.__abctx.drawImage(window.__abimg.map[id], x, y, w, h);  // honors current transform
});
EM_JS(void, ab_releaseImage, (int id), {
    if (window.__abimg && window.__abimg.map[id]) delete window.__abimg.map[id];
});

namespace artboard
{
    void Canvas2DTarget::save() { ab_save(); }
    void Canvas2DTarget::restore() { ab_restore(); }
    void Canvas2DTarget::setTransform(const Transform &t) { ab_xform(t.a, t.b, t.c, t.d, t.e, t.f); }
    void Canvas2DTarget::clipRect(double x, double y, double w, double h) { ab_clip(x, y, w, h); }
    void Canvas2DTarget::clipPath() { ab_clipPath(); }
    void Canvas2DTarget::pushLayer(double alpha) { ab_pushLayer(alpha); }
    void Canvas2DTarget::popLayer() { ab_popLayer(); }
    void Canvas2DTarget::setFill(const Color &c) { ab_fillStyle(c.r, c.g, c.b, c.a); }
    void Canvas2DTarget::setRadialFill(double cx, double cy, double radius, const Color &inner, const Color &outer)
    {
        ab_radialFill(cx, cy, radius, inner.r, inner.g, inner.b, inner.a, outer.r, outer.g, outer.b, outer.a);
    }
    void Canvas2DTarget::setLinearFill(double x0, double y0, double x1, double y1, const Color &start, const Color &end)
    {
        ab_linearFill(x0, y0, x1, y1, start.r, start.g, start.b, start.a, end.r, end.g, end.b, end.a);
    }
    void Canvas2DTarget::setStroke(const Color &c, double width) { ab_strokeStyle(c.r, c.g, c.b, c.a, width); }
    void Canvas2DTarget::beginPath() { ab_begin(); }
    void Canvas2DTarget::moveTo(double x, double y) { ab_moveTo(x, y); }
    void Canvas2DTarget::lineTo(double x, double y) { ab_lineTo(x, y); }
    void Canvas2DTarget::quadTo(double cx, double cy, double x, double y) { ab_quad(cx, cy, x, y); }
    void Canvas2DTarget::cubicTo(double a, double b, double c, double d, double x, double y) { ab_cubic(a, b, c, d, x, y); }
    void Canvas2DTarget::closePath() { ab_close(); }
    void Canvas2DTarget::fillPath() { ab_fill(); }
    void Canvas2DTarget::strokePath() { ab_stroke(); }
    void Canvas2DTarget::drawText(const std::string &text, double x, double y, double sizePx,
                                   const std::string &fontFamily, double letterSpacingPx)
    {
        ab_text(text.c_str(), x, y, sizePx, fontFamily.c_str(), letterSpacingPx);
    }

    double Canvas2DTarget::measureText(const std::string &text, double sizePx,
                                        const std::string &fontFamily, double letterSpacingPx) const
    {
        return ab_measure_text(text.c_str(), sizePx, fontFamily.c_str(), letterSpacingPx);
    }
    int Canvas2DTarget::registerImage(const uint8_t *rgba, int w, int h)
    {
        return ab_registerImage(reinterpret_cast<uintptr_t>(rgba), w, h);
    }
    void Canvas2DTarget::updateImage(int id, const uint8_t *rgba, int w, int h)
    {
        ab_updateImage(id, reinterpret_cast<uintptr_t>(rgba), w, h);
    }
    void Canvas2DTarget::drawImage(int id, const Rect &dst) { ab_drawImage(id, dst.x, dst.y, dst.w, dst.h); }
    void Canvas2DTarget::releaseImage(int id) { ab_releaseImage(id); }
}
