#include "InputRouter.h"

namespace artboard
{
    InputTarget *InputRouter::topAt(const Point &p) const
    {
        // Later-added targets are on top: search from the back.
        for (auto it = mTargets.rbegin(); it != mTargets.rend(); ++it)
            if ((*it)->hitTest(p))
                return *it;
        return nullptr;
    }

    void InputRouter::route(const Gesture &g)
    {
        using T = Gesture::Type;
        switch (g.type)
        {
        case T::Down:
            mCapture = topAt(g.pos); // capture the pressed target for the whole press
            if (mCapture)
                mCapture->onGesture(g);
            break;

        case T::Move:
        case T::DragStart:
        case T::Drag:
            if (mCapture)
                mCapture->onGesture(g); // captured press receives moves even outside bounds
            else if (g.type == T::Move)
            {
                if (InputTarget *t = topAt(g.pos)) // hover when no press is active
                    t->onGesture(g);
            }
            break;

        case T::Up:
        case T::Drop:
            if (mCapture)
                mCapture->onGesture(g);
            mCapture = nullptr; // release capture at the end of the press
            break;

        case T::Click:
        case T::DoubleClick:
        case T::RightClick:
            if (InputTarget *t = topAt(g.pos)) // post-press: hit-test fresh
                t->onGesture(g);
            break;
        }
    }
}
