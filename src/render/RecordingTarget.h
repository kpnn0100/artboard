/*
 *  Arstro Artboard — RecordingTarget: an IRenderTarget that records every call as
 *  a DrawOp. Used by unit tests (assert the exact primitive stream) and as a
 *  serialization/inspection point. Platform-free, so it is fully testable.
 */
#pragma once
#include "RenderTarget.h"
#include <vector>

namespace artboard
{
    struct DrawOp
    {
        enum class Kind
        {
            Save, Restore, SetTransform, SetFill, SetStroke,
            BeginPath, MoveTo, LineTo, QuadTo, CubicTo, ClosePath,
            FillPath, StrokePath, DrawText
        };
        Kind kind;
        double args[6] = {0, 0, 0, 0, 0, 0};
        Color color;
        double width = 0;
        Transform transform;
        std::string text;
    };

    class RecordingTarget : public IRenderTarget
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

        const std::vector<DrawOp> &ops() const { return mOps; }
        void clear() { mOps.clear(); }
        int count(DrawOp::Kind k) const;

    private:
        std::vector<DrawOp> mOps;
    };
}
