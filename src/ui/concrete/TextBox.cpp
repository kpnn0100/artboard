#include "TextBox.h"

namespace artboard
{
    TextBox::TextBox(const TextBoxStyle &style)
        : mStyle(style)
    {
        focusable = true;
        width.set(180.0);
        height.set(34.0);
    }

    void TextBox::setStyle(const TextBoxStyle &style)
    {
        mStyle = style;
        syncVisuals();
    }

    void TextBox::render(IRenderTarget &t, const Transform &parent) const
    {
        syncVisuals();
        Segment::render(t, parent);
    }

    bool TextBox::handleGesture(const Gesture &g, const Point &localPoint)
    {
        if (g.type == Gesture::Type::Down)
        {
            requestFocus();
            return true;
        }
        return Segment::handleGesture(g, localPoint);
    }

    bool TextBox::handleKey(const KeyEvent &event)
    {
        if (readOnly)
            return false;
        if (event.type == KeyEvent::Type::Text && !event.text.empty())
        {
            text += event.text;
            return true;
        }
        if (event.type == KeyEvent::Type::Down && event.keyCode == 8 && !text.empty())
        {
            text.pop_back();
            return true;
        }
        return Segment::handleKey(event);
    }

    void TextBox::ensureVisualTree() const
    {
        if (mBox)
            return;

        auto self = const_cast<TextBox *>(this);
        self->mBox = std::make_shared<RectangleSegment>();
        self->mLabel = std::make_shared<LabelSegment>();
        self->mCaret = std::make_shared<RectangleSegment>();
        self->mBox->inputTransparent = true;
        self->mLabel->inputTransparent = true;
        self->mCaret->inputTransparent = true;
        self->addChild(self->mBox);
        self->addChild(self->mLabel);
        self->addChild(self->mCaret);
    }

    void TextBox::syncVisuals() const
    {
        ensureVisualTree();

        const double padding = 10.0;
        mBox->style = hasFocus() ? mStyle.focused : mStyle.idle;
        mBox->width.set(width.value());
        mBox->height.set(height.value());

        mLabel->text = text.empty() ? placeholder : text;
        mLabel->style = text.empty() ? mStyle.placeholder : mStyle.text;
        mLabel->x.set(padding);
        mLabel->y.set((height.value() - mLabel->style.sizePx) * 0.5 - 2.0);

        mCaret->style = {Paint::filled(mStyle.caretColor), 0.0};
        mCaret->visible = hasFocus();
        mCaret->x.set(padding + estimateTextWidth(text, mStyle.text.sizePx));
        mCaret->y.set(8.0);
        mCaret->width.set(2.0);
        mCaret->height.set(height.value() - 16.0);
    }

    double TextBox::estimateTextWidth(const std::string &value, double sizePx) const
    {
        return sizePx * 0.6 * static_cast<double>(value.size());
    }
}
