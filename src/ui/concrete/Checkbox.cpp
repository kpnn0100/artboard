#include "Checkbox.h"

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
        mBox->style = mStyle.box;
        mBox->width.set(side);
        mBox->height.set(side);

        mIndicator->style = mStyle.indicator;
        mIndicator->visible = mChecked;
        mIndicator->x.set(5.0);
        mIndicator->y.set(5.0);
        mIndicator->width.set(side - 10.0);
        mIndicator->height.set(side - 10.0);

        mLabel->text = text;
        mLabel->style = mStyle.label;
        mLabel->x.set(side + 10.0);
        mLabel->y.set((height.value() - mStyle.label.sizePx) * 0.5 - 2.0);
    }

    void Checkbox::toggle()
    {
        mChecked = !mChecked;
    }
}
