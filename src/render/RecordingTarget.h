/*
 *  Arstro Artboard — RecordingTarget: an IRenderTarget that records every call as
 *  a DrawOp. Used by unit tests (assert the exact primitive stream) and as a
 *  serialization/inspection point. Platform-free, so it is fully testable.
 */
#pragma once
#include "RenderTarget.h"
#include <vector>
#include <cstdint>

namespace artboard
{
    struct DrawOp
    {
        enum class Kind
        {
            Save, Restore, SetTransform, ClipRect, ClipPath, PushLayer, PopLayer,
            SetFill, SetRadialFill, SetLinearFill, SetStroke,
            BeginPath, MoveTo, LineTo, QuadTo, CubicTo, ClosePath,
            FillPath, StrokePath, DrawText,
            RegisterImage, UpdateImage, DrawImage, ReleaseImage
        };
        Kind kind;
        double args[6] = {0, 0, 0, 0, 0, 0};  // DrawImage uses args[0..3] = dst x,y,w,h
        Color color;       // SetFill / SetStroke colour, or gradient start/inner colour
        Color color2;      // gradient end/outer colour
        double width = 0;
        Transform transform;
        std::string text;
        std::string fontFamily;     // DrawText: requested font family ("" = adapter default)
        double letterSpacingPx = 0; // DrawText: extra advance between glyphs (0 = normal)
        // raster image ops
        int imageId = 0;            // assigned id (register) or target id (update/draw/release)
        int imgW = 0, imgH = 0;     // register/update dimensions
        uint64_t pixelHash = 0;     // register/update byte sum (prove pixels changed; not stored verbatim)
    };

    class RecordingTarget : public IRenderTarget
    {
    public:
        void save() override;
        void restore() override;
        void setTransform(const Transform &t) override;
        void clipRect(double x, double y, double w, double h) override;
        void clipPath() override;
        void pushLayer(double alpha) override;
        void popLayer() override;
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

        const std::vector<DrawOp> &ops() const { return mOps; }
        void clear() { mOps.clear(); }
        int count(DrawOp::Kind k) const;

    private:
        static uint64_t hashPixels(const uint8_t *rgba, int w, int h);
        std::vector<DrawOp> mOps;
        int mNextImageId = 1;
    };
}
