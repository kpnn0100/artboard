/*
 *  Arstro Artboard — TextBox: focusable single-line text entry.
 *
 *  A real field, not a place characters accumulate: a caret that blinks and can be placed
 *  (FR-38), a selection that can be dragged, double-clicked, or extended with shift (FR-44),
 *  clipboard copy/cut/paste through the platform-free Clipboard seam, word-wise motion and
 *  deletion, and a view that scrolls to keep the caret visible (FR-39).
 */
#pragma once
#include "../base/Clipboard.h"
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
        /** Put the caret here and COLLAPSE the selection — what "click here" means. Use
         *  setSelection() to place the caret while keeping an anchor. */
        void setCaret(int byteOffset);
        /** Put the caret at the end — the natural place after setting `text` in code. */
        void caretToEnd() { setCaret((int)text.size()); }

        // ---- selection (FR-44) ----
        /** The selection is the range between the ANCHOR and the caret; empty when they
         *  coincide. Both ends always sit on codepoint boundaries. */
        int anchor() const { return mAnchor; }
        int selectionStart() const { return mAnchor < mCaret_ ? mAnchor : mCaret_; }
        int selectionEnd() const { return mAnchor < mCaret_ ? mCaret_ : mAnchor; }
        bool hasSelection() const { return mAnchor != mCaret_; }
        std::string selectedText() const;
        void setSelection(int anchorByte, int caretByte);
        void selectAll();
        void clearSelection() { mAnchor = mCaret_; }

        // ---- editing (each replaces a non-empty selection) ----
        /** Insert at the caret, replacing any selection. No-op while readOnly. */
        void insertText(const std::string &s);
        /** Erase the selection; the caret and anchor land at its start. */
        void deleteSelection();

        // ---- clipboard (FR-44) ----
        /** Copy the selection. Allowed while readOnly: reading is not mutation. */
        void copy() const;
        void cut();
        void paste();

        /** Blink half-period. Any caret move or edit restarts the cycle showing. */
        static constexpr double kBlinkMs = 530.0;

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
        /** Clamp into range and walk onto a codepoint boundary. The one place either end of
         *  the selection is normalised, so no path can leave one mid-glyph. */
        int boundary(int byteOffset) const;
        /** One shared definition of "word" for Ctrl+Arrow, Ctrl+Backspace and double-click. */
        int wordLeft(int from) const;
        int wordRight(int from) const;
        /** The run under `at`: word characters, or the run of separators when in whitespace. */
        void wordAt(int at, int &from, int &to) const;
        /** The byte offset nearest a local x, on a codepoint boundary. */
        int offsetAtX(double localX) const;
        /** Move the caret; `extend` keeps the anchor (shift-select), else collapses. */
        void moveCaret(int to, bool extend);
        /** Restart the blink cycle showing — called by every move and every edit. */
        void resetBlink() { mBlinkT0 = mNowMs; }
        bool caretVisible() const;

        TextBoxStyle mStyle;
        int mCaret_ = 0;
        int mAnchor = 0;          // the fixed end of the selection (FR-44)
        double mBlinkT0 = 0.0;
        mutable double mScrollX = 0.0;   // horizontal text offset that keeps the caret visible (FR-39)
        mutable IRenderTarget *mMeasure = nullptr;  // last target seen, for accurate caret placement
        double mNowMs = 0.0;
        bool mFocusPrev = false;
        Property mFocusAmt{0.0};  // animated focus factor (border blend + caret fade)
        mutable std::shared_ptr<RectangleSegment> mBox;
        mutable std::shared_ptr<LabelSegment> mLabel;
        mutable std::shared_ptr<RectangleSegment> mSelection;   // the band behind the glyphs
        mutable std::shared_ptr<RectangleSegment> mCaret;
    };
}
