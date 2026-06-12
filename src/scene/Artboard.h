/*
 *  Arstro Artboard — the scene root. Holds the canvas size, a background, and an
 *  ordered list of drawables. render() optionally advances animation (onFrame),
 *  paints the background, then draws the children in order.
 */
#pragma once
#include "Drawable.h"
#include "Shapes.h"
#include <functional>
#include <memory>
#include <vector>

namespace artboard
{
    class Artboard
    {
    public:
        explicit Artboard(const Size &size = Size{0, 0}) : mSize(size) {}

        Size size() const { return mSize; }
        void setSize(const Size &s) { mSize = s; }
        void setBackground(const Color &c) { mBackground = c; mHasBackground = true; }

        void add(std::shared_ptr<Drawable> d) { mChildren.push_back(std::move(d)); }
        int childCount() const { return (int)mChildren.size(); }

        /** Per-frame animation hook: called with the current time before drawing. */
        void setOnFrame(std::function<void(double)> fn) { mOnFrame = std::move(fn); }

        /** Advance animation (if any) and render the whole scene to `t`. */
        void render(IRenderTarget &t, double nowMs = 0.0);

    private:
        Size mSize;
        Color mBackground;
        bool mHasBackground = false;
        std::vector<std::shared_ptr<Drawable>> mChildren;
        std::function<void(double)> mOnFrame;
    };
}
