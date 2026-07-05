#pragma once
#include "../../render/RenderTarget.h"
#include <cairo/cairo.h>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace artboard
{
    class CairoTarget : public IRenderTarget
    {
    public:
        CairoTarget() = default;
        explicit CairoTarget(cairo_t *context) : mContext(context) {}
        ~CairoTarget() override;

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
        void drawText(const std::string &text, double x, double y, double sizePx,
                      const std::string &fontFamily = "", double letterSpacingPx = 0.0) override;
        int registerImage(const uint8_t *rgba, int w, int h) override;
        void updateImage(int id, const uint8_t *rgba, int w, int h) override;
        void drawImage(int id, const Rect &dst) override;
        void releaseImage(int id) override;

    private:
        // A registered image: a Cairo ARGB32 surface backed by an owned byte buffer.
        // cairo_image_surface_create_for_data does NOT copy, so the buffer must
        // outlive the surface — both live here.
        struct ImageEntry
        {
            cairo_surface_t *surface = nullptr;
            std::vector<uint8_t> data;  // BGRA8 premultiplied, Cairo stride
            int w = 0, h = 0;
        };
        void buildEntry(ImageEntry &e, const uint8_t *rgba, int w, int h);

        cairo_t *mContext = nullptr;
        std::unordered_map<int, ImageEntry> mImages;
        int mNextImageId = 1;
    };
}