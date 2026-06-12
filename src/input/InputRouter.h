/*
 *  Arstro Artboard — input routing: hit-test + press capture.
 *
 *  InputTarget is anything that can be hit and handle gestures. RectTarget is a
 *  convenience target (a bounds rect + a handler). InputRouter z-orders targets,
 *  delivers a press's whole sequence (Down -> Move/Drag -> Up/Drop) to the target
 *  captured on Down, and hit-tests post-press gestures (Click/DoubleClick/
 *  RightClick) fresh.
 */
#pragma once
#include "Input.h"
#include <functional>
#include <vector>

namespace artboard
{
    struct InputTarget
    {
        virtual ~InputTarget() = default;
        virtual bool hitTest(const Point &p) const = 0;
        virtual void onGesture(const Gesture &g) = 0;
    };

    class RectTarget : public InputTarget
    {
    public:
        Rect bounds;
        std::function<void(const Gesture &)> handler;
        RectTarget() = default;
        RectTarget(const Rect &r, std::function<void(const Gesture &)> h) : bounds(r), handler(std::move(h)) {}
        bool hitTest(const Point &p) const override { return bounds.contains(p); }
        void onGesture(const Gesture &g) override { if (handler) handler(g); }
    };

    class InputRouter
    {
    public:
        /** Targets added later are on top (hit-tested first). */
        void add(InputTarget *t) { mTargets.push_back(t); }
        void clear() { mTargets.clear(); mCapture = nullptr; }

        /** Route one gesture to the appropriate target. */
        void route(const Gesture &g);

        InputTarget *captured() const { return mCapture; }

    private:
        InputTarget *topAt(const Point &p) const;
        std::vector<InputTarget *> mTargets;
        InputTarget *mCapture = nullptr;
    };
}
