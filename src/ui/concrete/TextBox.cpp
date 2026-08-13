#include "TextBox.h"
#include <cmath>
#include "../base/Clipboard.h"
#include "../../anim/Motion.h"
#include <algorithm>
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

    int TextBox::boundary(int byteOffset) const
    {
        const int n = (int)text.size();
        int c = byteOffset < 0 ? 0 : (byteOffset > n ? n : byteOffset);
        // Never land inside a multi-byte codepoint.
        while (c > 0 && c < n && ((unsigned char)text[(size_t)c] & 0xC0) == 0x80)
            --c;
        return c;
    }

    void TextBox::setCaret(int byteOffset)
    {
        // Placing the caret COLLAPSES the selection — leaving a stale anchor behind would mean
        // the next Backspace silently deleted a range the user never selected.
        mCaret_ = mAnchor = boundary(byteOffset);
        resetBlink();
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

    // ---- selection (FR-44) ----

    void TextBox::setSelection(int anchorByte, int caretByte)
    {
        mAnchor = boundary(anchorByte);   // the same normalisation for BOTH ends
        mCaret_ = boundary(caretByte);
        resetBlink();
    }

    void TextBox::selectAll() { setSelection(0, (int)text.size()); }

    std::string TextBox::selectedText() const
    {
        return text.substr((size_t)selectionStart(), (size_t)(selectionEnd() - selectionStart()));
    }

    void TextBox::deleteSelection()
    {
        if (!hasSelection() || readOnly)
            return;
        const int from = selectionStart();
        text.erase((size_t)from, (size_t)(selectionEnd() - from));
        mCaret_ = mAnchor = from;
        resetBlink();
    }

    void TextBox::insertText(const std::string &s)
    {
        if (readOnly)
            return;
        deleteSelection();
        text.insert((size_t)mCaret_, s);
        mCaret_ = mAnchor = boundary(mCaret_ + (int)s.size());
        resetBlink();
    }

    void TextBox::moveCaret(int to, bool extend)
    {
        mCaret_ = boundary(to);
        if (!extend)
            mAnchor = mCaret_;   // a plain move collapses; shift keeps the anchor
        resetBlink();
    }

    // ---- clipboard (FR-44) ----

    void TextBox::copy() const
    {
        if (hasSelection())                 // reading is not mutation: allowed while readOnly
            Clipboard::write(selectedText());
    }

    void TextBox::cut()
    {
        if (readOnly || !hasSelection())
            return;
        Clipboard::write(selectedText());
        deleteSelection();
    }

    void TextBox::paste()
    {
        const std::string s = Clipboard::read();
        if (!s.empty())
            insertText(s);
    }

    // ---- words: one definition, shared by Ctrl+Arrow, Ctrl+Delete and double-click ----

    namespace
    {
        bool isWordByte(unsigned char c)
        {
            return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
                   c == '_' || c >= 0x80;   // treat multi-byte codepoints as word content
        }
    }

    int TextBox::wordLeft(int from) const
    {
        int i = from;
        while (i > 0 && !isWordByte((unsigned char)text[(size_t)(i - 1)])) --i;
        while (i > 0 && isWordByte((unsigned char)text[(size_t)(i - 1)])) --i;
        return i;
    }

    int TextBox::wordRight(int from) const
    {
        const int n = (int)text.size();
        int i = from;
        while (i < n && !isWordByte((unsigned char)text[(size_t)i])) ++i;
        while (i < n && isWordByte((unsigned char)text[(size_t)i])) ++i;
        return i;
    }

    void TextBox::wordAt(int at, int &from, int &to) const
    {
        const int n = (int)text.size();
        if (n == 0) { from = to = 0; return; }
        int i = at >= n ? n - 1 : at;
        // In whitespace, select the run of whitespace; otherwise the run of word characters.
        const bool word = isWordByte((unsigned char)text[(size_t)i]);
        from = i;
        to = i;
        while (from > 0 && isWordByte((unsigned char)text[(size_t)(from - 1)]) == word) --from;
        while (to < n && isWordByte((unsigned char)text[(size_t)to]) == word) ++to;
    }

    bool TextBox::caretVisible() const
    {
        if (reducedMotion())
            return true;   // a blinking caret is motion; the switch turns motion off
        const double phase = std::fmod(mNowMs - mBlinkT0, kBlinkMs * 2.0);
        return phase >= 0.0 ? phase < kBlinkMs : phase + kBlinkMs * 2.0 < kBlinkMs;
    }

    void TextBox::resolvePendingPointer() const
    {
        if (mPending == Pending::None)
            return;
        auto self = const_cast<TextBox *>(this);
        const int at = offsetAtX(mPendingX);
        switch (mPending)
        {
        case Pending::Place: self->moveCaret(at, false); break;
        case Pending::Extend: self->moveCaret(at, true); break;
        case Pending::Word:
        {
            int from = 0, to = 0;
            wordAt(at, from, to);
            self->setSelection(from, to);
            break;
        }
        case Pending::None: break;
        }
        mPending = Pending::None;
    }

    int TextBox::offsetAtX(double localX) const
    {
        const double padding = 10.0;
        const double target = localX - padding + mScrollX;
        int best = 0;
        double bestDist = -1.0;
        for (int b = 0;; b = stepRight(b))
        {
            const double d = std::fabs(textWidthTo(b) - target);
            if (bestDist < 0.0 || d < bestDist)
            {
                bestDist = d;
                best = b;
            }
            if (b >= (int)text.size())
                break;
        }
        return best;
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
        switch (g.type)
        {
        // These only RECORD the pointer position: turning it into a caret offset needs
        // measureText, which is valid during a render and not from an input callback.
        case Gesture::Type::Down:
            requestFocus();
            // Shift-press EXTENDS from the existing anchor; a plain press collapses.
            mPending = g.shift ? Pending::Extend : Pending::Place;
            mPendingX = localPoint.x;
            resetBlink();
            return true;

        case Gesture::Type::DragStart:
        case Gesture::Type::Drag:
            // Dragging from the press extends continuously; the anchor stays where the press
            // put it, which is what makes a drag select a range rather than move the caret.
            mPending = Pending::Extend;
            mPendingX = localPoint.x;
            resetBlink();
            return true;

        case Gesture::Type::DoubleClick:
            mPending = Pending::Word;
            mPendingX = localPoint.x;
            resetBlink();
            return true;
        default:
            break;
        }
        return Segment::handleGesture(g, localPoint);
    }

    bool TextBox::handleKey(const KeyEvent &event)
    {
        // `text` may have been assigned from outside since the last key: re-normalise BOTH
        // ends without collapsing, or a programmatic edit would drop the user's selection.
        mCaret_ = boundary(mCaret_);
        mAnchor = boundary(mAnchor);

        if (event.type == KeyEvent::Type::Down)
        {
            // Motion and copy work while readOnly: inspecting is not mutating (FR-38).
            switch (event.keyCode)
            {
            case 37:   // Left
                moveCaret(event.ctrl ? wordLeft(mCaret_)
                                     : (hasSelection() && !event.shift ? selectionStart()
                                                                       : stepLeft(mCaret_)),
                          event.shift);
                return true;
            case 39:   // Right
                moveCaret(event.ctrl ? wordRight(mCaret_)
                                     : (hasSelection() && !event.shift ? selectionEnd()
                                                                       : stepRight(mCaret_)),
                          event.shift);
                return true;
            case 36: moveCaret(0, event.shift); return true;                      // Home
            case 35: moveCaret((int)text.size(), event.shift); return true;       // End
            case 65:                                                              // A
                if (event.ctrl) { selectAll(); return true; }
                break;
            case 67:                                                              // C
                if (event.ctrl) { copy(); return true; }
                break;
            case 88:                                                              // X
                if (event.ctrl) { cut(); return true; }
                break;
            case 86:                                                              // V
                if (event.ctrl) { paste(); return true; }
                break;
            default:
                break;
            }
        }

        if (readOnly)
            return false;

        if (event.type == KeyEvent::Type::Text && !event.text.empty())
        {
            insertText(event.text);            // replaces the selection, if any
            return true;
        }
        if (event.type == KeyEvent::Type::Down && event.keyCode == 8)              // Backspace
        {
            if (hasSelection()) { deleteSelection(); return true; }
            if (mCaret_ == 0) return true;
            const int from = event.ctrl ? wordLeft(mCaret_) : stepLeft(mCaret_);
            text.erase((size_t)from, (size_t)(mCaret_ - from));
            mCaret_ = mAnchor = from;
            resetBlink();
            return true;
        }
        if (event.type == KeyEvent::Type::Down && event.keyCode == 46)             // Delete
        {
            if (hasSelection()) { deleteSelection(); return true; }
            if (mCaret_ >= (int)text.size()) return true;
            const int to = event.ctrl ? wordRight(mCaret_) : stepRight(mCaret_);
            text.erase((size_t)mCaret_, (size_t)(to - mCaret_));
            resetBlink();
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
        self->mSelection = std::make_shared<RectangleSegment>();
        self->mLabel = std::make_shared<LabelSegment>();
        self->mCaret = std::make_shared<RectangleSegment>();
        self->mBox->inputTransparent = true;
        self->mSelection->inputTransparent = true;
        self->mLabel->inputTransparent = true;
        self->mCaret->inputTransparent = true;
        // Order is the z-order: the highlight sits BEHIND the glyphs, the caret in front.
        self->addChild(self->mBox);
        self->addChild(self->mSelection);
        self->addChild(self->mLabel);
        self->addChild(self->mCaret);
    }

    void TextBox::syncVisuals() const
    {
        ensureVisualTree();
        // A render is the only place text measurement is valid, so this is where a recorded
        // click becomes a caret offset.
        resolvePendingPointer();
        mBox->visible = drawsBuiltInVisuals;   // FR-41
        mLabel->visible = drawsBuiltInVisuals;
        mCaret->visible = drawsBuiltInVisuals;
        if (!drawsBuiltInVisuals)
            return;

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
        if (hasFocus())
        {
            // Follow the caret by the smallest shift that brings it back inside the field.
            if (caretX - mScrollX > visible) mScrollX = caretX - visible;
            if (caretX - mScrollX < 0.0) mScrollX = caretX;
        }
        else
        {
            // An unfocused field reads from its START. It must not keep following a caret
            // that is not there — a column of values would all show their tail ends.
            mScrollX = 0.0;
        }
        const double fullW = textWidthTo((int)text.size());
        mScrollX = std::max(0.0, std::min(mScrollX, std::max(0.0, fullW - visible)));

        mLabel->x.set(padding - mScrollX);
        mLabel->y.set((height.value() - mLabel->style.sizePx) * 0.5 - 2.0);

        // Selection band, behind the glyphs, in the same scrolled frame as the label.
        const bool showSelection = hasSelection() && hasFocus();
        mSelection->visible = showSelection;
        if (showSelection)
        {
            const double x0 = textWidthTo(selectionStart()) - mScrollX;
            const double x1 = textWidthTo(selectionEnd()) - mScrollX;
            Color band = dimColor(mStyle.selectionColor, dim);
            band.a *= fa;
            mSelection->style = {Paint::filled(band), 0.0};
            mSelection->x.set(padding + x0);
            mSelection->y.set((height.value() - mStyle.text.sizePx * 1.35) * 0.5);
            mSelection->width.set(std::max(0.0, x1 - x0));
            mSelection->height.set(mStyle.text.sizePx * 1.35);
        }

        // A text cursor: 1px wide, ~1.25x the text size tall, vertically centred — and
        // BLINKING, restarted showing by every move and every edit so it is never dark at the
        // moment it moves (FR-44). Reduced motion keeps it steady.
        Color caret = dimColor(mStyle.caretColor, dim);
        caret.a *= fa;  // caret fades in with focus, out on blur
        mCaret->style = {Paint::filled(caret), 0.0};
        mCaret->visible = fa > 0.01 && caretVisible();
        // Drawn AT the caret position, not always at the end (FR-38), in the scrolled frame.
        const double caretH = mStyle.text.sizePx * 1.25;
        mCaret->x.set(padding + caretX - mScrollX);
        mCaret->y.set((height.value() - caretH) * 0.5);
        mCaret->width.set(1.0);
        mCaret->height.set(caretH);
    }

    double TextBox::estimateTextWidth(const std::string &value, double sizePx) const
    {
        return sizePx * 0.6 * static_cast<double>(value.size());
    }
}
