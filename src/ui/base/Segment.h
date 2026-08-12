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

        // ---- group opacity (FR-32) ----
        // Fades this segment AND its whole child subtree as ONE composited group via the
        // HAL's pushLayer/popLayer (FR-27), so overlapping descendants blend at full opacity
        // first and only the combined result fades. Animate it to show/hide instead of
        // flipping `visible` (FR-25). <= kOpacityEpsilon means "gone": not drawn, not hit-tested.
        Property opacity{1.0};
        static constexpr double kOpacityEpsilon = 1e-3;

        // ---- animated transform channel (FR-33) ----
        // Rotation (radians) and scale are applied ABOUT (pivotX, pivotY) in local space, so a
        // segment spins/pops around its own centre by setting the pivot to half its size. The
        // free `Drawable::transform` field stays the innermost, caller-owned transform.
        Property rotation{0.0};
        Property scaleX{1.0};
        Property scaleY{1.0};
        Property pivotX{0.0};
        Property pivotY{0.0};

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

        /** True while this segment is effectively invisible (hidden or fully transparent):
         *  it is neither drawn nor hit-tested. */
        bool isFadedOut() const { return !visible || opacity.value() <= kOpacityEpsilon; }

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
        // ---- disabled state (FR-40) ----
        /** Animated disabled factor in [0,1]; controls dim their look by it, so switching
         *  `enabled` fades rather than flipping. */
        double disabledAmount() const { return mDisabledAmount.value(); }

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
        /** Signal hooks (FR-36): overridable notifications a subclass reacts to, on top of
         *  the std::function callbacks a *caller* subscribes to. These are the authoring
         *  seam — a generated or hand-written subclass starts its animations here. */
        virtual void onHoverChanged(bool hovered) { (void)hovered; }
        virtual void onFocusChanged(bool focused) { (void)focused; }

        virtual bool hitTestSelf(const Point &localPoint) const;
        virtual bool handleGesture(const Gesture &g, const Point &localPoint);
        virtual bool handleKey(const KeyEvent &event);

    private:
        void renderContent(IRenderTarget &t, const Transform &parent) const;
        void renderOverlayContent(IRenderTarget &t, const Transform &parent) const;
        Segment *topmostChildAt(const Point &worldPoint) const;
        bool dispatchGesture(const Gesture &g);
        /** Drop this segment from the focus registry. `notify` fires onFocusChanged(false);
         *  the destructor passes false — a dying object gets no signals. */
        void clearFocusRegistration(bool notify = true);
        void updateHoverAnim(double nowMs);
        void updateDisabledAnim(double nowMs);
        static bool isHorizontal(SnapEdge e);
        double edgeInset(SnapEdge e) const;
        void resolveSnap();

        Segment *mParent = nullptr;
        bool mHovered = false;
        bool mHoverPrev = false;
        Property mHoverAmount{0.0};
        bool mEnabledPrev = true;
        Property mDisabledAmount{0.0};
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