/*
 *  Arstro Artboard — ImageView: displays a raster photo, aspect-fitted into its
 *  bounds. The only core consumer of the IRenderTarget raster primitive. Owns a
 *  copy of the pixels (so it can re-register if drawn into a different target —
 *  e.g. a RecordingTarget in tests, then Cairo/Canvas2D at runtime), registers
 *  lazily, and re-uploads only when the pixels change. Platform-free: it emits
 *  only register/update/draw HAL calls.
 */
#pragma once
#include "../base/Segment.h"
#include <cstdint>
#include <vector>

namespace artboard
{
    class ImageView : public Segment
    {
    public:
        enum class Fit { Contain, Cover, Fill };

        /** Set the displayed pixels (straight RGBA8, top-down). Copies the bytes. */
        void setImage(const uint8_t *rgba, int w, int h);
        void clearImage();
        bool hasImage() const { return mW > 0 && mH > 0; }
        int imageWidth() const { return mW; }
        int imageHeight() const { return mH; }

        void setFit(Fit f) { mFit = f; }
        /** Destination rect (local space) the image is drawn into for the current fit. */
        Rect fittedRect() const;

    protected:
        void onPaint(IRenderTarget &t) const override;
        bool hitTestSelf(const Point &) const override { return false; }  // display-only

    private:
        std::vector<uint8_t> mPixels;
        int mW = 0, mH = 0;
        Fit mFit = Fit::Contain;
        mutable int mId = 0;
        mutable bool mDirty = false;
        mutable IRenderTarget *mLastTarget = nullptr;
    };
}
