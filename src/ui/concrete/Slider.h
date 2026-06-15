/*
 *  Arstro Artboard — Slider: a horizontal ranged control (track + fill + thumb).
 */
#pragma once
#include "../base/Segment.h"
#include "../base/Theme.h"
#include "../base/AbstractSlider.h"
#include "../base/RectangleSegment.h"
#include "../base/CircleSegment.h"

namespace artboard
{
    class Slider : public Segment, public AbstractSlider
    {
    public:
        explicit Slider(const SliderStyle &style = Theme::basicTheme().slider);

        void setStyle(const SliderStyle &style);
        const SliderStyle &style() const { return mStyle; }
        void render(IRenderTarget &t, const Transform &parent = Transform::identity()) const override;

    protected:
        bool handleGesture(const Gesture &g, const Point &localPoint) override;
        bool handleKey(const KeyEvent &event) override;

    private:
        void ensureVisualTree() const;
        void syncVisuals() const;
        double valueForLocalX(double localX) const;

        SliderStyle mStyle;
        mutable std::shared_ptr<RectangleSegment> mTrack;
        mutable std::shared_ptr<RectangleSegment> mRangeFill;
        mutable std::shared_ptr<CircleSegment> mThumb;
    };
}
