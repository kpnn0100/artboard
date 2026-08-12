#include "Checkbox.h"
#include "../base/Interaction.h"

namespace artboard
{
    Checkbox::Checkbox(std::string label, const CheckboxStyle &style)
        : text(std::move(label)), mStyle(style)
    {
        focusable = true;
        width.set(150.0);
        height.set(26.0);
    }

    void Checkbox::setStyle(const CheckboxStyle &style)
    {
        mStyle = style;
        syncVisuals();
    }

    void Checkbox::render(IRenderTarget &t, const Transform &parent) const
    {
        syncVisuals();
        Segment::render(t, parent);
    }

    void Checkbox::advance(double nowMs)
    {
        mNowMs = nowMs;
        mCheck.update(nowMs);
        Segment::advance(nowMs); // drives hoverAmount()
    }

    bool Checkbox::handleGesture(const Gesture &g, const Point &localPoint)
    {
        if (g.type == Gesture::Type::Click)
        {
            toggle();
            return true;
        }
        return Segment::handleGesture(g, localPoint);
    }

    bool Checkbox::handleKey(const KeyEvent &event)
    {
        if (isConfirmKey(event))
        {
            toggle();
            return true;
        }
        return Segment::handleKey(event);
    }

    void Checkbox::ensureVisualTree() const
    {
        if (mBox)
            return;

        auto self = const_cast<Checkbox *>(this);
        self->mBox = std::make_shared<RectangleSegment>();
        self->mIndicator = std::make_shared<RectangleSegment>();
        self->mLabel = std::make_shared<LabelSegment>();
        self->mBox->inputTransparent = true;
        self->mIndicator->inputTransparent = true;
        self->mLabel->inputTransparent = true;
        self->addChild(self->mBox);
        self->addChild(self->mIndicator);
        self->addChild(self->mLabel);
    }

    void Checkbox::syncVisuals() const
    {
        ensureVisualTree();

        const double side = height.value();
        // Hover: brighten the box and pull its border toward the accent (indicator fill).
        const double dim = disabledAmount();
        mBox->style = dimBox(hoverBox(mStyle.box, mStyle.indicator.paint.fill, hoverAmount()), dim);
        mBox->width.set(side);
        mBox->height.set(side);

        // The check indicator grows in/out from the centre (it never pops).
        const double c = mCheck.value();
        const double isz = (side - 10.0) * c;
        mIndicator->style = dimBox(mStyle.indicator, dim);
        mIndicator->visible = c > 0.001;
        mIndicator->x.set((side - isz) * 0.5);
        mIndicator->y.set((side - isz) * 0.5);
        mIndicator->width.set(isz);
        mIndicator->height.set(isz);

        mLabel->text = text;
        mLabel->style = mStyle.label;
        mLabel->style.color = dimColor(mLabel->style.color, dim);
        mLabel->x.set(side + 10.0);
        mLabel->y.set((height.value() - mStyle.label.sizePx) * 0.5 - 2.0);
    }

    void Checkbox::toggle()
    {
        mChecked = !mChecked;
        mCheck.animateTo(mChecked ? 1.0 : 0.0, 140.0, Easing::EaseOutCubic, mNowMs);
        onCheckedChanged(mChecked);  // FR-36
    }
}
