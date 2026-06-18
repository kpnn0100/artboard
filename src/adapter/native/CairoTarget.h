#pragma once
#include "../../render/RenderTarget.h"
#include <cairo/cairo.h>

namespace artboard
{
    class CairoTarget : public IRenderTarget
    {
    public:
        CairoTarget() = default;
        explicit CairoTarget(cairo_t *context) : mContext(context) {}

        void setContext(cairo_t *context) { mContext = context; }
        cairo_t *context() const { return mContext; }

        void save() override;
        void restore() override;
        void setTransform(const Transform &t) override;
        void clipRect(double x, double y, double w, double h) override;
        void setFill(const Color &c) override;
        void setRadialFill(double cx, double cy, double radius, const Color &inner, const Color &outer) override;
        void setLinearFill(double x0, double y0, double x1, double y1, const Color &start, const Color &end) override;
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

    private:
        cairo_t *mContext = nullptr;
    };
}