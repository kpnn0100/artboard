#include "Button.h"
#include "../base/Interaction.h"

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

    void Button::advance(double nowMs)
    {
        mNowMs = nowMs;
        mPress.update(nowMs);
        Segment::advance(nowMs); // drives hoverAmount()
    }

    bool Button::handleGesture(const Gesture &g, const Point &localPoint)
    {
        if (g.type == Gesture::Type::Down)
        {
            mPressed = true;
            mPress.animateTo(1.0, 90.0, Easing::EaseOutCubic, mNowMs);
            onPressDown();
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
            mPress.animateTo(0.0, 150.0, Easing::EaseOutCubic, mNowMs);
            if (invoke)
            {
                onRelease();
                onClicked();
                if (onClick)
                    onClick();
            }
            return true;
        }
        if (g.type == Gesture::Type::Drop)
        {
            const bool wasPressed = mPressed;
            mPressed = false;
            mPress.animateTo(0.0, 150.0, Easing::EaseOutCubic, mNowMs);
            if (wasPressed)
                onCancel();
            return true;
        }
        return Segment::handleGesture(g, localPoint);
    }

    bool Button::handleKey(const KeyEvent &event)
    {
        if (isConfirmKey(event))
        {
            onClicked();  // FR-36: keyboard confirm is a click too
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

        // Crossfade idle<->pressed by the animated press factor; hover nudges the body
        // partway toward the pressed (accent) look — both eased, so nothing snaps. Both
        // factors are non-overshooting EaseOutCubic values in [0,1], so blend stays in [0,1].
        const double press = mPress.value();
        const double blend = press + (1.0 - press) * 0.4 * hoverAmount();
        const double dim = disabledAmount();
        mBody->style = dimBox(lerpBox(mStyle.idle, mStyle.pressed, blend), dim);
        mBody->x.set(0.0);
        mBody->y.set(0.0);
        mBody->width.set(width.value());
        mBody->height.set(height.value());

        mLabel->text = text;
        mLabel->style = mStyle.label;
        mLabel->style.color = dimColor(mLabel->style.color, dim);
        mLabel->x.set(10.0);
        mLabel->y.set((height.value() - mStyle.label.sizePx) * 0.5 - 2.0);
    }
}
