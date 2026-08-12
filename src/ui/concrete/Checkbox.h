/*
 *  Arstro Artboard — Checkbox: boolean toggle (box + indicator + label).
 */
#pragma once
#include "../base/Segment.h"
#include "../base/Theme.h"
#include "../base/RectangleSegment.h"
#include "../base/LabelSegment.h"
#include <string>

namespace artboard
{
    class Checkbox : public Segment
    {
    public:
        explicit Checkbox(std::string label = {}, const CheckboxStyle &style = Theme::basicTheme().checkbox);

        std::string text;

        bool checked() const { return mChecked; }
        void setChecked(bool checked) { mChecked = checked; mCheck.set(checked ? 1.0 : 0.0); }
        void setStyle(const CheckboxStyle &style);
        void render(IRenderTarget &t, const Transform &parent = Transform::identity()) const override;
        void advance(double nowMs) override;

    protected:
        /** Signal hook (FR-36): the checked state changed, by pointer or keyboard.
         *  Not fired by the programmatic setChecked(). */
        virtual void onCheckedChanged(bool checked) { (void)checked; }

        bool handleGesture(const Gesture &g, const Point &localPoint) override;
        bool handleKey(const KeyEvent &event) override;

    private:
        void ensureVisualTree() const;
        void syncVisuals() const;
        void toggle();

        CheckboxStyle mStyle;
        bool mChecked = false;
        double mNowMs = 0.0;
        Property mCheck{0.0};  // animated check grow-in factor [0,1]
        mutable std::shared_ptr<RectangleSegment> mBox;
        mutable std::shared_ptr<RectangleSegment> mIndicator;
        mutable std::shared_ptr<LabelSegment> mLabel;
    };
}
