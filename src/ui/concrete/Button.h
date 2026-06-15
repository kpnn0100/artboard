/*
 *  Arstro Artboard — Button: press/click/keyboard-confirm control (body + label).
 */
#pragma once
#include "../base/Segment.h"
#include "../base/Theme.h"
#include "../base/RectangleSegment.h"
#include "../base/LabelSegment.h"
#include <functional>
#include <string>

namespace artboard
{
    class Button : public Segment
    {
    public:
        explicit Button(std::string label = {}, const ButtonStyle &style = Theme::basicTheme().button);

        std::string text;
        std::function<void()> onClick;

        void setStyle(const ButtonStyle &style);
        void render(IRenderTarget &t, const Transform &parent = Transform::identity()) const override;

    protected:
        bool handleGesture(const Gesture &g, const Point &localPoint) override;
        bool handleKey(const KeyEvent &event) override;

    private:
        void ensureVisualTree() const;
        void syncVisuals() const;

        ButtonStyle mStyle;
        bool mPressed = false;
        mutable std::shared_ptr<RectangleSegment> mBody;
        mutable std::shared_ptr<LabelSegment> mLabel;
    };
}
