#include "Canvas2DTarget.h"
#include <emscripten.h>

// Each primitive maps to one Canvas2D call on the active context (window.__abctx,
// set by the page before rendering). Colors become CSS rgba() strings.
EM_JS(void, ab_save, (), { window.__abctx.save(); });
EM_JS(void, ab_restore, (), { window.__abctx.restore(); });
EM_JS(void, ab_xform, (double a, double b, double c, double d, double e, double f),
      { window.__abctx.setTransform(a, b, c, d, e, f); });
EM_JS(void, ab_fillStyle, (double r, double g, double b, double a),
      { window.__abctx.fillStyle = 'rgba(' + (r * 255 | 0) + ',' + (g * 255 | 0) + ',' + (b * 255 | 0) + ',' + a + ')'; });
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
EM_JS(void, ab_text, (const char *s, double x, double y, double size),
      { var c = window.__abctx; c.font = size + 'px sans-serif'; c.fillText(UTF8ToString(s), x, y); });

namespace artboard
{
    void Canvas2DTarget::save() { ab_save(); }
    void Canvas2DTarget::restore() { ab_restore(); }
    void Canvas2DTarget::setTransform(const Transform &t) { ab_xform(t.a, t.b, t.c, t.d, t.e, t.f); }
    void Canvas2DTarget::setFill(const Color &c) { ab_fillStyle(c.r, c.g, c.b, c.a); }
    void Canvas2DTarget::setStroke(const Color &c, double width) { ab_strokeStyle(c.r, c.g, c.b, c.a, width); }
    void Canvas2DTarget::beginPath() { ab_begin(); }
    void Canvas2DTarget::moveTo(double x, double y) { ab_moveTo(x, y); }
    void Canvas2DTarget::lineTo(double x, double y) { ab_lineTo(x, y); }
    void Canvas2DTarget::quadTo(double cx, double cy, double x, double y) { ab_quad(cx, cy, x, y); }
    void Canvas2DTarget::cubicTo(double a, double b, double c, double d, double x, double y) { ab_cubic(a, b, c, d, x, y); }
    void Canvas2DTarget::closePath() { ab_close(); }
    void Canvas2DTarget::fillPath() { ab_fill(); }
    void Canvas2DTarget::strokePath() { ab_stroke(); }
    void Canvas2DTarget::drawText(const std::string &text, double x, double y, double sizePx) { ab_text(text.c_str(), x, y, sizePx); }
}
