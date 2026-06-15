/*
 *  Arstro Artboard — Knob: a rotary analog control (vertical drag), onChange(value).
 */
#pragma once
#include "../base/Segment.h"
#include "../base/Theme.h"
#include "../base/AbstractSlider.h"
#include <functional>
#include <string>

namespace artboard
{
    class Knob : public Segment, public AbstractSlider
    {
    public:
        explicit Knob(const KnobStyle &style = Theme::basicTheme().knob);

        std::string label;
        double sensitivity = 160.0; // px of vertical drag for the full range
        std::function<void(double)> onChange;

        void setStyle(const KnobStyle &style) { mStyle = style; }
        const KnobStyle &style() const { return mStyle; }

    protected:
        void onPaint(IRenderTarget &t) const override;
        bool handleGesture(const Gesture &g, const Point &localPoint) override;
        bool handleKey(const KeyEvent &event) override;

    private:
        void emitChange();
        KnobStyle mStyle;
        double mDragStartValue = 0.0;
    };
}
