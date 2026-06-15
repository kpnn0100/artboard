#include "Button.h"

namespace artboard
{
    Button::Button(std::string label, const ButtonStyle &style)
        : text(std::move(label)), mStyle(style)
    {
        focusable = true;
        width.set(120.0);
        height.set(32.0);
    }

    void Button::setStyle(const ButtonStyle &style)
    {
        mStyle = style;
        syncVisuals();
    }

    void Button::render(IRenderTarget &t, const Transform &parent) const
    {
        syncVisuals();
        Segment::render(t, parent);
    }

    bool Button::handleGesture(const Gesture &g, const Point &localPoint)
    {
        if (g.type == Gesture::Type::Down)
        {
            mPressed = true;
            return true;
        }
        if (g.type == Gesture::Type::Up)
        {
            return true;
        }
        if (g.type == Gesture::Type::Click)
        {
            const bool invoke = mPressed;
            mPressed = false;
            if (invoke && onClick)
                onClick();
            return true;
        }
        if (g.type == Gesture::Type::Drop)
        {
            mPressed = false;
            return true;
        }
        return Segment::handleGesture(g, localPoint);
    }

    bool Button::handleKey(const KeyEvent &event)
    {
        if (isConfirmKey(event))
        {
            if (onClick)
                onClick();
            return true;
        }
        return Segment::handleKey(event);
    }

    void Button::ensureVisualTree() const
    {
        if (mBody)
            return;

        auto self = const_cast<Button *>(this);
        self->mBody = std::make_shared<RectangleSegment>();
        self->mLabel = std::make_shared<LabelSegment>();
        self->mBody->inputTransparent = true;
        self->mLabel->inputTransparent = true;
        self->addChild(self->mBody);
        self->addChild(self->mLabel);
    }

    void Button::syncVisuals() const
    {
        ensureVisualTree();

        mBody->style = mPressed ? mStyle.pressed : mStyle.idle;
        mBody->x.set(0.0);
        mBody->y.set(0.0);
        mBody->width.set(width.value());
        mBody->height.set(height.value());

        mLabel->text = text;
        mLabel->style = mStyle.label;
        mLabel->x.set(10.0);
        mLabel->y.set((height.value() - mStyle.label.sizePx) * 0.5 - 2.0);
    }
}
