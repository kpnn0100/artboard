/*
 *  Arstro Artboard — Knob: a rotary analog control (vertical drag), onChange(value).
 */
#pragma once
#include "../base/Segment.h"
#include "../base/Theme.h"
#include "../base/AbstractSlider.h"
#include "../base/ModBus.h"
#include <functional>
#include <string>
#include <vector>

namespace artboard
{
    /** One modulation routing on a Knob: a source (by id) scaled by a signed depth,
     *  drawn as a coloured outer ring. depth is a fraction of the knob's full range. */
    struct KnobMod
    {
        int sourceId = 0;
        double depth = 0.0; // [-1,1] of the full range
        Color color;
        bool bipolar = false; // true: source swings [-1,1] -> ring spans base ± |depth|
    };

    class Knob : public Segment, public AbstractSlider
    {
    public:
        explicit Knob(const KnobStyle &style = Theme::basicTheme().knob);

        std::string label;
        double sensitivity = 160.0; // px of vertical drag for the full range
        std::function<void(double)> onChange;

        void setStyle(const KnobStyle &style) { mStyle = style; }
        const KnobStyle &style() const { return mStyle; }

        // ---- modulation (Serum-style depth rings) ----
        // The ModBus supplies the live source values; targets read it to draw rings
        // and compute the modulated value. addModulation appends a routing (or, if the
        // source is already routed, just re-colours it). Each routing renders as an
        // outer ring; dragging that ring (vertical) sets its depth.
        void setModBus(const ModBus *bus) { mBus = bus; }
        void addModulation(int sourceId, const Color &color, double depth = 0.25, bool bipolar = false);
        void setModDepth(int sourceId, double depth);
        void clearModulations() { mMods.clear(); }
        const std::vector<KnobMod> &modulations() const { return mMods; }
        /** base value + Σ depthᵢ·sourceᵢ·range, clamped to the range. */
        double modulatedValue() const;

        // Eases the displayed value toward the target each frame (smooth knob motion).
        void advance(double nowMs) override;

    protected:
        void onPaint(IRenderTarget &t) const override;
        bool handleGesture(const Gesture &g, const Point &localPoint) override;
        bool handleKey(const KeyEvent &event) override;

    private:
        void emitChange();
        double displayNormalized() const; // smoothed value mapped to [0,1]
        double dialRadius() const;         // dial radius from the current size
        int ringAtRadius(double rad) const; // mod index whose ring band holds rad, else -1
        KnobStyle mStyle;
        double mDragStartValue = 0.0;
        int mDragRing = -1;          // mod index being depth-dragged, or -1 = value drag
        double mDragStartDepth = 0.0;
        // Smoothed display value (spring toward the real value); mutable so onPaint can
        // lazily seed it before the first advance().
        mutable double mDisplay = 0.0;
        mutable bool mDisplayInit = false;
        double mVel = 0.0;
        double mLastMs = -1.0;
        std::vector<KnobMod> mMods;
        const ModBus *mBus = nullptr;
    };
}
