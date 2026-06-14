#include "Segment.h"
#include <unordered_map>

namespace artboard
{
    namespace
    {
        std::unordered_map<int, Segment *> &focusRegistry()
        {
            static std::unordered_map<int, Segment *> registry;
            return registry;
        }
    }

    Segment::~Segment()
    {
        clearFocusRegistration();
    }

    void Segment::render(IRenderTarget &t, const Transform &parent) const
    {
        if (!visible)
            return;

        const Transform world = parent.mul(localTransform());
        t.save();
        t.setTransform(world);
        onPaint(t);
        t.restore();

        for (const auto &child : mChildren)
            child->render(t, world);
    }

    bool Segment::hitTest(const Point &p) const
    {
        if (!visible || !enabled)
            return false;

        for (auto it = mChildren.rbegin(); it != mChildren.rend(); ++it)
            if ((*it)->hitTest(p))
                return true;

        if (inputTransparent)
            return false;

        return hitTestSelf(toLocal(p));
    }

    void Segment::onGesture(const Gesture &g)
    {
        dispatchGesture(g);
    }

    void Segment::addChild(std::shared_ptr<Segment> child)
    {
        if (!child)
            return;
        child->mParent = this;
        mChildren.push_back(std::move(child));
    }

    void Segment::clearChildren()
    {
        for (auto &child : mChildren)
            child->mParent = nullptr;
        mChildren.clear();
        mCapturedChild = nullptr;
    }

    Transform Segment::localTransform() const
    {
        return Transform::translation(x.value(), y.value()).mul(transform);
    }

    Transform Segment::worldTransform() const
    {
        if (!mParent)
            return localTransform();
        return mParent->worldTransform().mul(localTransform());
    }

    Point Segment::toLocal(const Point &worldPoint) const
    {
        return worldTransform().inverse().apply(worldPoint);
    }

    void Segment::advance(double nowMs)
    {
        x.update(nowMs);
        y.update(nowMs);
        width.update(nowMs);
        height.update(nowMs);
        for (const auto &child : mChildren)
            child->advance(nowMs);
    }

    void Segment::requestFocus()
    {
        if (!focusable)
            return;

        auto &registry = focusRegistry();
        auto found = registry.find(focusIndex);
        if (found != registry.end() && found->second && found->second != this)
            found->second->mFocused = false;

        registry[focusIndex] = this;
        mFocused = true;
    }

    Segment *Segment::focusedInGroup(int focusIndex)
    {
        auto &registry = focusRegistry();
        auto found = registry.find(focusIndex);
        return found == registry.end() ? nullptr : found->second;
    }

    bool Segment::dispatchKey(const KeyEvent &event)
    {
        for (auto it = mChildren.rbegin(); it != mChildren.rend(); ++it)
            if ((*it)->dispatchKey(event))
                return true;

        return mFocused ? handleKey(event) : false;
    }

    bool Segment::hitTestSelf(const Point &localPoint) const
    {
        return localBounds().contains(localPoint);
    }

    bool Segment::handleGesture(const Gesture &g, const Point &localPoint)
    {
        if (!enabled || inputTransparent)
            return false;

        if (focusable && g.type == Gesture::Type::Down)
            requestFocus();

        return mInputController ? mInputController->onGesture(*this, g, localPoint) : false;
    }

    bool Segment::handleKey(const KeyEvent &event)
    {
        if (!enabled)
            return false;
        return mInputController ? mInputController->onKey(*this, event) : false;
    }

    Segment *Segment::topmostChildAt(const Point &worldPoint) const
    {
        for (auto it = mChildren.rbegin(); it != mChildren.rend(); ++it)
            if ((*it)->hitTest(worldPoint))
                return it->get();
        return nullptr;
    }

    bool Segment::dispatchGesture(const Gesture &g)
    {
        if (!visible || !enabled)
            return false;

        const bool releaseCapture = g.type == Gesture::Type::Up || g.type == Gesture::Type::Drop;
        switch (g.type)
        {
        case Gesture::Type::Down:
        {
            mCapturedChild = topmostChildAt(g.pos);
            if (mCapturedChild)
                return mCapturedChild->dispatchGesture(g);
            return handleGesture(g, toLocal(g.pos));
        }
        case Gesture::Type::DragStart:
        case Gesture::Type::Drag:
        case Gesture::Type::Up:
        case Gesture::Type::Move:
        case Gesture::Type::Drop:
            if (mCapturedChild)
            {
                const bool handled = mCapturedChild->dispatchGesture(g);
                if (releaseCapture)
                    mCapturedChild = nullptr;
                return handled;
            }
            break;
        case Gesture::Type::Click:
        case Gesture::Type::DoubleClick:
        case Gesture::Type::RightClick:
            if (Segment *child = topmostChildAt(g.pos))
                return child->dispatchGesture(g);
            break;
        }

        const bool handled = handleGesture(g, toLocal(g.pos));
        if (releaseCapture)
            mCapturedChild = nullptr;
        return handled;
    }

    void Segment::clearFocusRegistration()
    {
        auto &registry = focusRegistry();
        auto found = registry.find(focusIndex);
        if (found != registry.end() && found->second == this)
            registry.erase(found);
        mFocused = false;
    }
}