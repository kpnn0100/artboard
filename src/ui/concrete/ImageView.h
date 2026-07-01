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
        /** Destination rect (local space) the image is drawn into for the current fit,
         *  including the current zoom + pan. */
        Rect fittedRect() const;

        // ── zoom / pan (for ctrl-scroll magnification) ──
        double zoom() const { return mZoom; }
        void resetView() { mZoom = 1.0; mPanX = mPanY = 0.0; }
        /** Multiply the zoom by `factor` (clamped 1..8) keeping the image point under
         *  `local` fixed; pan is clamped so the image still covers the view. */
        void zoomAbout(double factor, const Point &local);
        /** Translate the view by (dx,dy) local pixels, clamped to keep it covered. */
        void panBy(double dx, double dy);

    protected:
        void onPaint(IRenderTarget &t) const override;
        // Display-only at 1x (click-through); interactive when zoomed (drag pans).
        bool hitTestSelf(const Point &p) const override { return mZoom > 1.0 && localBounds().contains(p); }
        bool handleGesture(const Gesture &g, const Point &local) override;

    private:
        Rect baseFittedRect() const;  // aspect-fit rect at zoom 1, no pan

        std::vector<uint8_t> mPixels;
        int mW = 0, mH = 0;
        Fit mFit = Fit::Contain;
        double mZoom = 1.0, mPanX = 0.0, mPanY = 0.0;
        void clampPan();              // keep the zoomed image covering the view
        Point mDragLast{0, 0};        // previous drag position (for pan deltas)
        mutable int mId = 0;
        mutable bool mDirty = false;
        mutable IRenderTarget *mLastTarget = nullptr;
    };
}
