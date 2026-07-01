#include "ImageView.h"

namespace artboard
{
    void ImageView::setImage(const uint8_t *rgba, int w, int h)
    {
        if (!rgba || w <= 0 || h <= 0)
        {
            clearImage();
            return;
        }
        mPixels.assign(rgba, rgba + (size_t)w * h * 4);
        mW = w;
        mH = h;
        mDirty = true;
    }

    void ImageView::clearImage()
    {
        mPixels.clear();
        mW = mH = 0;
        mDirty = false;
        mId = 0;  // adapter reclaims on its own teardown
    }

    Rect ImageView::baseFittedRect() const
    {
        const double bw = width.value(), bh = height.value();
        if (mW <= 0 || mH <= 0 || bw <= 0 || bh <= 0)
            return Rect{0, 0, 0, 0};
        if (mFit == Fit::Fill)
            return Rect{0, 0, bw, bh};
        const double sx = bw / mW, sy = bh / mH;
        const double s = (mFit == Fit::Cover) ? (sx > sy ? sx : sy)   // fill, crop overflow
                                              : (sx < sy ? sx : sy);  // contain, letterbox
        const double w = mW * s, h = mH * s;
        return Rect{(bw - w) * 0.5, (bh - h) * 0.5, w, h};
    }

    Rect ImageView::fittedRect() const
    {
        const Rect b = baseFittedRect();
        if (mZoom == 1.0) return b;
        const double w = b.w * mZoom, h = b.h * mZoom;
        return Rect{b.x - (w - b.w) * 0.5 + mPanX, b.y - (h - b.h) * 0.5 + mPanY, w, h};
    }

    void ImageView::zoomAbout(double factor, const Point &local)
    {
        const Rect before = fittedRect();
        double nz = mZoom * factor;
        if (nz < 1.0) nz = 1.0; else if (nz > 8.0) nz = 8.0;
        // normalized image point currently under `local`
        const double u = before.w > 0 ? (local.x - before.x) / before.w : 0.5;
        const double v = before.h > 0 ? (local.y - before.y) / before.h : 0.5;
        mZoom = nz;
        const Rect base = baseFittedRect();
        const double nw = base.w * mZoom, nh = base.h * mZoom;
        // place the same normalized point back under `local` (invert fittedRect's math)
        mPanX = local.x - u * nw - base.x + (nw - base.w) * 0.5;
        mPanY = local.y - v * nh - base.y + (nh - base.h) * 0.5;
        clampPan();
    }

    void ImageView::clampPan()
    {
        if (mZoom == 1.0) { mPanX = 0; mPanY = 0; return; }
        // keep the zoomed image covering the view (no gaps at the edges)
        const double bw = width.value(), bh = height.value();
        Rect f = fittedRect();
        if (f.x > 0) mPanX -= f.x;
        if (f.x + f.w < bw) mPanX += bw - (f.x + f.w);
        if (f.y > 0) mPanY -= f.y;
        if (f.y + f.h < bh) mPanY += bh - (f.y + f.h);
    }

    void ImageView::panBy(double dx, double dy)
    {
        if (mZoom == 1.0) return;
        mPanX += dx;
        mPanY += dy;
        clampPan();
    }

    bool ImageView::handleGesture(const Gesture &g, const Point &local)
    {
        if (mZoom <= 1.0)
            return Segment::handleGesture(g, local);
        switch (g.type)
        {
        case Gesture::Type::Down:
        case Gesture::Type::DragStart:
            mDragLast = local;
            return true;
        case Gesture::Type::Drag:
            panBy(local.x - mDragLast.x, local.y - mDragLast.y);
            mDragLast = local;
            return true;
        case Gesture::Type::Up:
        case Gesture::Type::Drop:
            return true;
        default:
            return Segment::handleGesture(g, local);
        }
    }

    void ImageView::onPaint(IRenderTarget &t) const
    {
        if (!hasImage())
            return;
        if (mLastTarget != &t || mId == 0)
        {
            mId = t.registerImage(mPixels.data(), mW, mH);
            mLastTarget = &t;
            mDirty = false;
        }
        else if (mDirty)
        {
            t.updateImage(mId, mPixels.data(), mW, mH);
            mDirty = false;
        }
        // clip to the view so a zoomed image never spills into the rest of the UI
        t.save();
        t.clipRect(0, 0, width.value(), height.value());
        t.drawImage(mId, fittedRect());
        t.restore();
    }
}
