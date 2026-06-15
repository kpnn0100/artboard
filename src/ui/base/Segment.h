#pragma once
#include "InputController.h"
#include "Property.h"
#include "../../input/InputRouter.h"
#include "../../scene/Drawable.h"
#include <memory>
#include <vector>

namespace artboard
{
    class Segment : public Drawable, public InputTarget
    {
    public:
        Segment() = default;
        ~Segment() override;

        Property x{0.0};
        Property y{0.0};
        Property width{0.0};
        Property height{0.0};
        bool enabled = true;
        bool focusable = false;
        bool clipToBounds = false;
        bool inputTransparent = false;
        int focusIndex = 0;

        void render(IRenderTarget &t, const Transform &parent = Transform::identity()) const override;
        bool hitTest(const Point &p) const override;
        void onGesture(const Gesture &g) override;

        void addChild(std::shared_ptr<Segment> child);
        void clearChildren();
        int childCount() const { return (int)mChildren.size(); }
        const std::vector<std::shared_ptr<Segment>> &children() const { return mChildren; }

        void setInputController(std::shared_ptr<InputController> controller) { mInputController = std::move(controller); }
        std::shared_ptr<InputController> inputController() const { return mInputController; }

        Rect localBounds() const { return Rect{0.0, 0.0, width.value(), height.value()}; }
        Transform localTransform() const;
        Transform worldTransform() const;
        Point toLocal(const Point &worldPoint) const;

        virtual void advance(double nowMs);
        void requestFocus();
        bool hasFocus() const { return mFocused; }
        static Segment *focusedInGroup(int focusIndex);
        bool dispatchKey(const KeyEvent &event);

    protected:
        void onDraw(IRenderTarget &) const override {}
        virtual void onPaint(IRenderTarget &t) const {}
        virtual bool hitTestSelf(const Point &localPoint) const;
        virtual bool handleGesture(const Gesture &g, const Point &localPoint);
        virtual bool handleKey(const KeyEvent &event);

    private:
        Segment *topmostChildAt(const Point &worldPoint) const;
        bool dispatchGesture(const Gesture &g);
        void clearFocusRegistration();

        Segment *mParent = nullptr;
        Segment *mCapturedChild = nullptr;
        bool mFocused = false;
        std::shared_ptr<InputController> mInputController;
        std::vector<std::shared_ptr<Segment>> mChildren;
    };
}