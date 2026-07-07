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
        /** Second render pass over the whole tree, AFTER render(), for content that
         *  must escape clipping and sit on top of everything (open dropdowns/popups).
         *  The app calls it once on the root after render(). Unclipped by design. */
        void renderOverlay(IRenderTarget &t, const Transform &parent = Transform::identity()) const;
        bool hitTest(const Point &p) const override;
        void onGesture(const Gesture &g) override;

        void addChild(std::shared_ptr<Segment> child);
        void clearChildren();
        /** Move this segment to the end of its parent's child list (drawn last among
         *  siblings, hit-tested first) — e.g. when a popup opens. */
        void raise();
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

        // ---- hover (FR-24) ----
        /** True while the pointer rests over this segment (single global owner). */
        bool isHovered() const { return mHovered; }
        /** Animated hover factor in [0,1]; controls scale their hover look by it. */
        double hoverAmount() const { return mHoverAmount.value(); }
        /** True if this segment OR any descendant is the current hover owner — for
         *  containers (e.g. ScrollView) that react to hover over their content. */
        bool isHoverWithin() const;
        /** Set the one hovered segment; clears the previously-hovered one. */
        static void setHovered(Segment *seg);
        static Segment *hoveredSegment();

    protected:
        void onDraw(IRenderTarget &) const override {}
        virtual void onPaint(IRenderTarget &t) const {}
        /** Drawn in the overlay pass (on top of everything, unclipped). Default none. */
        virtual void onOverlay(IRenderTarget &t) const {}
        virtual bool hitTestSelf(const Point &localPoint) const;
        virtual bool handleGesture(const Gesture &g, const Point &localPoint);
        virtual bool handleKey(const KeyEvent &event);

    private:
        Segment *topmostChildAt(const Point &worldPoint) const;
        bool dispatchGesture(const Gesture &g);
        void clearFocusRegistration();
        void updateHoverAnim(double nowMs);
        static bool isHorizontal(SnapEdge e);
        double edgeInset(SnapEdge e) const;
        void resolveSnap();

        Segment *mParent = nullptr;
        bool mHovered = false;
        bool mHoverPrev = false;
        Property mHoverAmount{0.0};
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