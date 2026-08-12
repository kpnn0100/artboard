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
        void advance(double nowMs) override;

    protected:
        // ---- signal hooks (FR-36) ----
        // The authoring seam: a subclass overrides these to drive its own motion. They fire
        // alongside (not instead of) the public `onClick` callback a caller subscribes to.
        virtual void onPressDown() {}
        virtual void onRelease() {}   // press completed into a click
        virtual void onCancel() {}    // press abandoned (dragged off / dropped elsewhere)
        virtual void onClicked() {}   // the button fired (pointer or keyboard)

        bool handleGesture(const Gesture &g, const Point &localPoint) override;
        bool handleKey(const KeyEvent &event) override;

    private:
        void ensureVisualTree() const;
        void syncVisuals() const;

        ButtonStyle mStyle;
        bool mPressed = false;
        double mNowMs = 0.0;
        mutable Property mPress{0.0};  // animated press factor (crossfade idle<->pressed)
        mutable std::shared_ptr<RectangleSegment> mBody;
        mutable std::shared_ptr<LabelSegment> mLabel;
    };
}
