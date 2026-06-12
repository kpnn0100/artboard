#include "RecordingTarget.h"

namespace artboard
{
    using K = DrawOp::Kind;

    void RecordingTarget::save() { mOps.push_back({K::Save}); }
    void RecordingTarget::restore() { mOps.push_back({K::Restore}); }

    void RecordingTarget::setTransform(const Transform &t)
    {
        DrawOp op{K::SetTransform};
        op.transform = t;
        mOps.push_back(op);
    }
    void RecordingTarget::setFill(const Color &c)
    {
        DrawOp op{K::SetFill};
        op.color = c;
        mOps.push_back(op);
    }
    void RecordingTarget::setStroke(const Color &c, double width)
    {
        DrawOp op{K::SetStroke};
        op.color = c;
        op.width = width;
        mOps.push_back(op);
    }
    void RecordingTarget::beginPath() { mOps.push_back({K::BeginPath}); }
    void RecordingTarget::moveTo(double x, double y)
    {
        DrawOp op{K::MoveTo};
        op.args[0] = x; op.args[1] = y;
        mOps.push_back(op);
    }
    void RecordingTarget::lineTo(double x, double y)
    {
        DrawOp op{K::LineTo};
        op.args[0] = x; op.args[1] = y;
        mOps.push_back(op);
    }
    void RecordingTarget::quadTo(double cx, double cy, double x, double y)
    {
        DrawOp op{K::QuadTo};
        op.args[0] = cx; op.args[1] = cy; op.args[2] = x; op.args[3] = y;
        mOps.push_back(op);
    }
    void RecordingTarget::cubicTo(double c1x, double c1y, double c2x, double c2y, double x, double y)
    {
        DrawOp op{K::CubicTo};
        op.args[0] = c1x; op.args[1] = c1y; op.args[2] = c2x; op.args[3] = c2y; op.args[4] = x; op.args[5] = y;
        mOps.push_back(op);
    }
    void RecordingTarget::closePath() { mOps.push_back({K::ClosePath}); }
    void RecordingTarget::fillPath() { mOps.push_back({K::FillPath}); }
    void RecordingTarget::strokePath() { mOps.push_back({K::StrokePath}); }
    void RecordingTarget::drawText(const std::string &text, double x, double y, double sizePx)
    {
        DrawOp op{K::DrawText};
        op.args[0] = x; op.args[1] = y; op.args[2] = sizePx;
        op.text = text;
        mOps.push_back(op);
    }

    int RecordingTarget::count(DrawOp::Kind k) const
    {
        int n = 0;
        for (const auto &op : mOps)
            if (op.kind == k)
                ++n;
        return n;
    }
}
