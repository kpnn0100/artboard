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
    void RecordingTarget::clipRect(double x, double y, double w, double h)
    {
        DrawOp op{K::ClipRect};
        op.args[0] = x; op.args[1] = y; op.args[2] = w; op.args[3] = h;
        mOps.push_back(op);
    }
    void RecordingTarget::clipPath() { mOps.push_back({K::ClipPath}); }
    void RecordingTarget::pushLayer(double alpha)
    {
        DrawOp op{K::PushLayer};
        op.args[0] = alpha;
        mOps.push_back(op);
    }
    void RecordingTarget::popLayer() { mOps.push_back({K::PopLayer}); }
    void RecordingTarget::setFill(const Color &c)
    {
        DrawOp op{K::SetFill};
        op.color = c;
        mOps.push_back(op);
    }
    void RecordingTarget::setRadialFill(double cx, double cy, double radius, const Color &inner, const Color &outer)
    {
        DrawOp op{K::SetRadialFill};
        op.args[0] = cx; op.args[1] = cy; op.args[2] = radius;
        op.color = inner;
        op.color2 = outer;
        mOps.push_back(op);
    }
    void RecordingTarget::setLinearFill(double x0, double y0, double x1, double y1, const Color &start, const Color &end)
    {
        DrawOp op{K::SetLinearFill};
        op.args[0] = x0; op.args[1] = y0; op.args[2] = x1; op.args[3] = y1;
        op.color = start;
        op.color2 = end;
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
    void RecordingTarget::drawText(const std::string &text, double x, double y, double sizePx,
                                    const std::string &fontFamily, double letterSpacingPx)
    {
        DrawOp op{K::DrawText};
        op.args[0] = x; op.args[1] = y; op.args[2] = sizePx;
        op.text = text;
        op.fontFamily = fontFamily;
        op.letterSpacingPx = letterSpacingPx;
        mOps.push_back(op);
    }

    uint64_t RecordingTarget::hashPixels(const uint8_t *rgba, int w, int h)
    {
        if (!rgba || w <= 0 || h <= 0)
            return 0;
        uint64_t sum = 0;
        const size_t n = (size_t)w * h * 4;
        for (size_t i = 0; i < n; ++i)
            sum += (uint64_t)(i + 1) * rgba[i];  // position-weighted so order matters
        return sum;
    }

    int RecordingTarget::registerImage(const uint8_t *rgba, int w, int h)
    {
        const int id = mNextImageId++;
        DrawOp op{K::RegisterImage};
        op.imageId = id;
        op.imgW = w; op.imgH = h;
        op.pixelHash = hashPixels(rgba, w, h);
        mOps.push_back(op);
        return id;
    }
    void RecordingTarget::updateImage(int id, const uint8_t *rgba, int w, int h)
    {
        DrawOp op{K::UpdateImage};
        op.imageId = id;
        op.imgW = w; op.imgH = h;
        op.pixelHash = hashPixels(rgba, w, h);
        mOps.push_back(op);
    }
    void RecordingTarget::drawImage(int id, const Rect &dst)
    {
        DrawOp op{K::DrawImage};
        op.imageId = id;
        op.args[0] = dst.x; op.args[1] = dst.y; op.args[2] = dst.w; op.args[3] = dst.h;
        mOps.push_back(op);
    }
    void RecordingTarget::releaseImage(int id)
    {
        DrawOp op{K::ReleaseImage};
        op.imageId = id;
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
