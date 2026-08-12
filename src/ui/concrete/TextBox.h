/*
 *  Arstro Artboard — TextBox: focusable single-line text entry (box + text + caret).
 */
#pragma once
#include "../base/Segment.h"
#include "../base/Theme.h"
#include "../base/RectangleSegment.h"
#include "../base/LabelSegment.h"
#include <string>

namespace artboard
{
    class TextBox : public Segment
    {
    public:
        explicit TextBox(const TextBoxStyle &style = Theme::basicTheme().textBox);

        std::string text;
        std::string placeholder;
        bool readOnly = false;

        /** Caret position as a BYTE offset into `text`, always on a codepoint boundary
         *  (FR-38). Clamped into range whenever `text` is changed from outside. */
        int caret() const { return mCaret_; }
        void setCaret(int byteOffset);
        /** Put the caret at the end — the natural place after setting `text` in code. */
        void caretToEnd() { setCaret((int)text.size()); }

        void setStyle(const TextBoxStyle &style);
        void render(IRenderTarget &t, const Transform &parent = Transform::identity()) const override;
        void advance(double nowMs) override;

    protected:
        bool handleGesture(const Gesture &g, const Point &localPoint) override;
        bool handleKey(const KeyEvent &event) override;

    private:
        void ensureVisualTree() const;
        void syncVisuals() const;
        double estimateTextWidth(const std::string &value, double sizePx) const;
        /** Advance width of the first `bytes` of `text`, via the target when one is
         *  available (accurate) and the estimate otherwise. */
        double textWidthTo(int bytes) const;
        /** Step a byte offset one whole UTF-8 codepoint left/right, clamped. */
        int stepLeft(int from) const;
        int stepRight(int from) const;

        TextBoxStyle mStyle;
        int mCaret_ = 0;
        mutable IRenderTarget *mMeasure = nullptr;  // last target seen, for accurate caret placement
        double mNowMs = 0.0;
        bool mFocusPrev = false;
        Property mFocusAmt{0.0};  // animated focus factor (border blend + caret fade)
        mutable std::shared_ptr<RectangleSegment> mBox;
        mutable std::shared_ptr<LabelSegment> mLabel;
        mutable std::shared_ptr<RectangleSegment> mCaret;
    };
}
