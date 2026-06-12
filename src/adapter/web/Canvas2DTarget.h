/*
 *  Arstro Artboard — web adapter. Implements the IRenderTarget HAL by driving an
 *  HTML Canvas2D context (via Emscripten). The active context is set from JS as
 *  `window.__abctx` before each frame. Compiled only by the web (emcc) build.
 */
#pragma once
#include "../../render/RenderTarget.h"

namespace artboard
{
    class Canvas2DTarget : public IRenderTarget
    {
    public:
        void save() override;
        void restore() override;
        void setTransform(const Transform &t) override;
        void setFill(const Color &c) override;
        void setStroke(const Color &c, double width) override;
        void beginPath() override;
        void moveTo(double x, double y) override;
        void lineTo(double x, double y) override;
        void quadTo(double cx, double cy, double x, double y) override;
        void cubicTo(double c1x, double c1y, double c2x, double c2y, double x, double y) override;
        void closePath() override;
        void fillPath() override;
        void strokePath() override;
        void drawText(const std::string &text, double x, double y, double sizePx) override;
    };
}
