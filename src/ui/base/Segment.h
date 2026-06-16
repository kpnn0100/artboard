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

        // ---- snap constraint ----
        // An edge of a segment. Left/Right/CenterX are horizontal (adjust x); Top/Bottom/
        // CenterY are vertical (adjust y).
        enum class SnapEdge { Left, Right, Top, Bottom, CenterX, CenterY };
        /** Glue myEdge to target's targetEdge + offset (parent space); resolved each advance().
         *  Ignored if target is null or this. */
        void snapTo(Segment *target, SnapEdge myEdge, SnapEdge targetEdge, double offset = 0.0);
        void clearSnap() { mSnapTarget = nullptr; }
        bool hasSnap() const { return mSnapTarget != nullptr; }
        /** Edge coordinate in the parent's space (from current x/y/width/height). */
        double edgeCoord(SnapEdge e) const;

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
        static bool isHorizontal(SnapEdge e);
        double edgeInset(SnapEdge e) const;
        void resolveSnap();

        Segment *mParent = nullptr;
        Segment *mSnapTarget = nullptr;
        SnapEdge mSnapMine = SnapEdge::Left;
        SnapEdge mSnapTheirs = SnapEdge::Right;
        double mSnapOffset = 0.0;
        Segment *mCapturedChild = nullptr;
        bool mFocused = false;
        std::shared_ptr<InputController> mInputController;
        std::vector<std::shared_ptr<Segment>> mChildren;
    };
}