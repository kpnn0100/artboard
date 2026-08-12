#include "TextBox.h"
#include <cmath>
#include "../base/Interaction.h"

namespace artboard
{
    TextBox::TextBox(const TextBoxStyle &style)
        : mStyle(style)
    {
        focusable = true;
        clipToBounds = true;   // FR-39: the value can never spill past the field
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
        mMeasure = &t;   // remember a real target so caret placement uses real metrics (FR-38)
        syncVisuals();
        Segment::render(t, parent);
    }

    // ---- caret (FR-38) ----

    void TextBox::setCaret(int byteOffset)
    {
        const int n = (int)text.size();
        int c = byteOffset < 0 ? 0 : (byteOffset > n ? n : byteOffset);
        // Never land inside a multi-byte codepoint.
        while (c > 0 && c < n && ((unsigned char)text[(size_t)c] & 0xC0) == 0x80)
            --c;
        mCaret_ = c;
    }

    int TextBox::stepLeft(int from) const
    {
        int c = from - 1;
        while (c > 0 && ((unsigned char)text[(size_t)c] & 0xC0) == 0x80)
            --c;
        return c < 0 ? 0 : c;
    }

    int TextBox::stepRight(int from) const
    {
        const int n = (int)text.size();
        int c = from + 1;
        while (c < n && ((unsigned char)text[(size_t)c] & 0xC0) == 0x80)
            ++c;
        return c > n ? n : c;
    }

    double TextBox::textWidthTo(int bytes) const
    {
        const int n = (int)text.size();
        const int b = bytes < 0 ? 0 : (bytes > n ? n : bytes);
        const std::string prefix = text.substr(0, (size_t)b);
        if (mMeasure)
            return mMeasure->measureText(prefix, mStyle.text.sizePx, mStyle.text.fontFamily,
                                         mStyle.text.letterSpacingPx);
        return estimateTextWidth(prefix, mStyle.text.sizePx);
    }

    void TextBox::advance(double nowMs)
    {
        mNowMs = nowMs;
        if (hasFocus() != mFocusPrev)  // ease the focus border + caret in/out (no pop)
        {
            mFocusPrev = hasFocus();
            mFocusAmt.animateTo(mFocusPrev ? 1.0 : 0.0, 140.0, Easing::EaseOutCubic, nowMs);
        }
        mFocusAmt.update(nowMs);
        Segment::advance(nowMs);  // drives hoverAmount()
    }

    bool TextBox::handleGesture(const Gesture &g, const Point &localPoint)
    {
        if (g.type == Gesture::Type::Down)
        {
            requestFocus();
            // Place the caret at the inter-character boundary nearest the pointer, measured
            // with the adapter's own metrics so it lands where the glyphs really are (FR-38).
            const double padding = 10.0;
            const double target = localPoint.x - padding;
            int best = 0;
            double bestDist = -1.0;
            for (int b = 0; b <= (int)text.size(); b = (b == (int)text.size()) ? b + 1 : stepRight(b))
            {
                if (b > (int)text.size())
                    break;
                const double d = std::fabs(textWidthTo(b) - target);
                if (bestDist < 0.0 || d < bestDist)
                {
                    bestDist = d;
                    best = b;
                }
                if (b == (int)text.size())
                    break;
            }
            setCaret(best);
            return true;
        }
        return Segment::handleGesture(g, localPoint);
    }

    bool TextBox::handleKey(const KeyEvent &event)
    {
        setCaret(mCaret_);   // `text` may have been assigned from outside since the last key

        // Caret movement works even when read-only: a value can be inspected without
        // being changed (FR-38).
        if (event.type == KeyEvent::Type::Down)
        {
            switch (event.keyCode)
            {
            case 37: setCaret(stepLeft(mCaret_)); return true;             // Left
            case 39: setCaret(stepRight(mCaret_)); return true;            // Right
            case 36: setCaret(0); return true;                             // Home
            case 35: setCaret((int)text.size()); return true;              // End
            default: break;
            }
        }
        if (readOnly)
            return false;
        if (event.type == KeyEvent::Type::Text && !event.text.empty())
        {
            text.insert((size_t)mCaret_, event.text);
            setCaret(mCaret_ + (int)event.text.size());
            return true;
        }
        if (event.type == KeyEvent::Type::Down && event.keyCode == 8 && mCaret_ > 0)
        {
            const int from = stepLeft(mCaret_);                            // Backspace
            text.erase((size_t)from, (size_t)(mCaret_ - from));
            setCaret(from);
            return true;
        }
        if (event.type == KeyEvent::Type::Down && event.keyCode == 46 && mCaret_ < (int)text.size())
        {
            const int to = stepRight(mCaret_);                             // Delete
            text.erase((size_t)mCaret_, (size_t)(to - mCaret_));
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
        const double dim = disabledAmount();
        // Blend idle<->focused by the animated focus factor, then hover brightens/pulls the
        // border toward the accent (caret colour). Neither the border nor caret pops.
        const double fa = mFocusAmt.value();
        mBox->style = dimBox(hoverBox(lerpBox(mStyle.idle, mStyle.focused, fa), mStyle.caretColor,
                                      hoverAmount()), dim);
        mBox->width.set(width.value());
        mBox->height.set(height.value());

        mLabel->text = text.empty() ? placeholder : text;
        mLabel->style = text.empty() ? mStyle.placeholder : mStyle.text;
        mLabel->style.color = dimColor(mLabel->style.color, dim);

        // FR-39: scroll the text so the caret is always inside the padded field, by the
        // smallest shift that achieves it. A value longer than the box stays editable
        // instead of spilling past it (the box also clips, as a backstop).
        const double visible = std::max(0.0, width.value() - padding * 2.0);
        const double caretX = textWidthTo(mCaret_);
        if (caretX - mScrollX > visible) mScrollX = caretX - visible;
        if (caretX - mScrollX < 0.0) mScrollX = caretX;
        const double fullW = textWidthTo((int)text.size());
        mScrollX = std::max(0.0, std::min(mScrollX, std::max(0.0, fullW - visible)));

        mLabel->x.set(padding - mScrollX);
        mLabel->y.set((height.value() - mLabel->style.sizePx) * 0.5 - 2.0);

        Color caret = dimColor(mStyle.caretColor, dim);
        caret.a *= fa;  // caret fades in with focus, out on blur
        mCaret->style = {Paint::filled(caret), 0.0};
        mCaret->visible = fa > 0.01;
        // Drawn AT the caret position, not always at the end (FR-38), in the scrolled frame.
        mCaret->x.set(padding + caretX - mScrollX);
        mCaret->y.set(8.0);
        mCaret->width.set(2.0);
        mCaret->height.set(height.value() - 16.0);
    }

    double TextBox::estimateTextWidth(const std::string &value, double sizePx) const
    {
        return sizePx * 0.6 * static_cast<double>(value.size());
    }
}
