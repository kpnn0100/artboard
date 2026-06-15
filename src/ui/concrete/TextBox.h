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

        void setStyle(const TextBoxStyle &style);
        void render(IRenderTarget &t, const Transform &parent = Transform::identity()) const override;

    protected:
        bool handleGesture(const Gesture &g, const Point &localPoint) override;
        bool handleKey(const KeyEvent &event) override;

    private:
        void ensureVisualTree() const;
        void syncVisuals() const;
        double estimateTextWidth(const std::string &value, double sizePx) const;

        TextBoxStyle mStyle;
        mutable std::shared_ptr<RectangleSegment> mBox;
        mutable std::shared_ptr<LabelSegment> mLabel;
        mutable std::shared_ptr<RectangleSegment> mCaret;
    };
}
