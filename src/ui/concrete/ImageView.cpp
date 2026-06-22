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

    Rect ImageView::fittedRect() const
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
        t.drawImage(mId, fittedRect());
    }
}
