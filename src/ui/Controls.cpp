#include "Controls.h"

namespace artboard
{
    namespace
    {
        void drawRoundedRect(IRenderTarget &t, const Rect &rect, double cornerRadius, const Paint &paint)
        {
            const double x = rect.x, y = rect.y, w = rect.w, h = rect.h;
            t.beginPath();
            if (cornerRadius > 0.0)
            {
                double radius = cornerRadius;
                const double half = (w < h ? w : h) * 0.5;
                if (radius > half)
                    radius = half;
                t.moveTo(x + radius, y);
                t.lineTo(x + w - radius, y);
                t.quadTo(x + w, y, x + w, y + radius);
                t.lineTo(x + w, y + h - radius);
                t.quadTo(x + w, y + h, x + w - radius, y + h);
                t.lineTo(x + radius, y + h);
                t.quadTo(x, y + h, x, y + h - radius);
                t.lineTo(x, y + radius);
                t.quadTo(x, y, x + radius, y);
                t.closePath();
            }
            else
            {
                t.moveTo(x, y);
                t.lineTo(x + w, y);
                t.lineTo(x + w, y + h);
                t.lineTo(x, y + h);
                t.closePath();
            }
            applyPaint(t, paint);
        }

        bool isConfirmKey(const KeyEvent &event)
        {
            return event.type == KeyEvent::Type::Down && (event.keyCode == 13 || event.keyCode == 32);
        }
    }

    void RectangleSegment::onPaint(IRenderTarget &t) const
    {
        drawRoundedRect(t, localBounds(), style.cornerRadius, style.paint);
    }

    void CircleSegment::onPaint(IRenderTarget &t) const
    {
        const double k = 0.5522847498307936;
        const double rx = width.value() * 0.5;
        const double ry = height.value() * 0.5;
        const double cx = rx;
        const double cy = ry;
        const double ox = rx * k;
        const double oy = ry * k;

        t.beginPath();
        t.moveTo(cx - rx, cy);
        t.cubicTo(cx - rx, cy - oy, cx - ox, cy - ry, cx, cy - ry);
        t.cubicTo(cx + ox, cy - ry, cx + rx, cy - oy, cx + rx, cy);
        t.cubicTo(cx + rx, cy + oy, cx + ox, cy + ry, cx, cy + ry);
        t.cubicTo(cx - ox, cy + ry, cx - rx, cy + oy, cx - rx, cy);
        t.closePath();
        applyPaint(t, style.paint);
    }

    void LabelSegment::onPaint(IRenderTarget &t) const
    {
        t.setFill(style.color);
        t.drawText(text, 0.0, style.sizePx, style.sizePx);
    }

    double AbstractSlider::clamp(double value) const
    {
        if (mMax < mMin)
            return mMin;
        if (value < mMin)
            return mMin;
        if (value > mMax)
            return mMax;
        return value;
    }

    void AbstractSlider::setRange(double minimum, double maximum)
    {
        mMin = minimum;
        mMax = maximum < minimum ? minimum : maximum;
        setValue(mValue);
    }

    void AbstractSlider::setValue(double value)
    {
        mValue = clamp(value);
    }

    double AbstractSlider::normalizedValue() const
    {
        const double span = mMax - mMin;
        if (span <= 0.0)
            return 0.0;
        return (mValue - mMin) / span;
    }

    Slider::Slider(const SliderStyle &style)
        : AbstractSlider(0.0, 0.0, 1.0), mStyle(style)
    {
        focusable = true;
        width.set(160.0);
        height.set(28.0);
    }

    void Slider::setStyle(const SliderStyle &style)
    {
        mStyle = style;
        syncVisuals();
    }

    void Slider::render(IRenderTarget &t, const Transform &parent) const
    {
        syncVisuals();
        Segment::render(t, parent);
    }

    bool Slider::handleGesture(const Gesture &g, const Point &localPoint)
    {
        if (g.type == Gesture::Type::Down || g.type == Gesture::Type::Drag || g.type == Gesture::Type::Click)
        {
            setValue(valueForLocalX(localPoint.x));
            return true;
        }
        return Segment::handleGesture(g, localPoint);
    }

    bool Slider::handleKey(const KeyEvent &event)
    {
        if (event.type != KeyEvent::Type::Down)
            return false;

        const double step = isAnalog() ? (maximum() - minimum()) / 20.0 : 1.0;
        if (event.keyCode == 37)
        {
            setValue(value() - step);
            return true;
        }
        if (event.keyCode == 39)
        {
            setValue(value() + step);
            return true;
        }
        return Segment::handleKey(event);
    }

    void Slider::ensureVisualTree() const
    {
        if (mTrack)
            return;

        auto self = const_cast<Slider *>(this);
        self->mTrack = std::make_shared<RectangleSegment>();
        self->mRangeFill = std::make_shared<RectangleSegment>();
        self->mThumb = std::make_shared<CircleSegment>();
        self->mTrack->inputTransparent = true;
        self->mRangeFill->inputTransparent = true;
        self->mThumb->inputTransparent = true;
        self->addChild(self->mTrack);
        self->addChild(self->mRangeFill);
        self->addChild(self->mThumb);
    }

    void Slider::syncVisuals() const
    {
        ensureVisualTree();

        const double trackHeight = height.value() * 0.35;
        const double trackY = (height.value() - trackHeight) * 0.5;
        const double normalized = normalizedValue();
        const double thumbDiameter = mStyle.thumbRadius * 2.0;
        const double thumbCenter = normalized * width.value();

        mTrack->style = mStyle.track;
        mTrack->x.set(0.0);
        mTrack->y.set(trackY);
        mTrack->width.set(width.value());
        mTrack->height.set(trackHeight);

        mRangeFill->style = mStyle.rangeFill;
        mRangeFill->x.set(0.0);
        mRangeFill->y.set(trackY);
        mRangeFill->width.set(width.value() * normalized);
        mRangeFill->height.set(trackHeight);

        mThumb->style = mStyle.thumb;
        mThumb->x.set(thumbCenter - thumbDiameter * 0.5);
        mThumb->y.set((height.value() - thumbDiameter) * 0.5);
        mThumb->width.set(thumbDiameter);
        mThumb->height.set(thumbDiameter);
    }

    double Slider::valueForLocalX(double localX) const
    {
        if (width.value() <= 0.0)
            return minimum();

        double normalized = localX / width.value();
        if (normalized < 0.0)
            normalized = 0.0;
        if (normalized > 1.0)
            normalized = 1.0;
        return minimum() + (maximum() - minimum()) * normalized;
    }

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