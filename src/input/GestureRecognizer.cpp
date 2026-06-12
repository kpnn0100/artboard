#include "GestureRecognizer.h"
#include <cmath>

namespace artboard
{
    static double dist(const Point &a, const Point &b)
    {
        return std::hypot(a.x - b.x, a.y - b.y);
    }

    void GestureRecognizer::emit(Gesture::Type t, const Point &pos, const Point &start, PointerButton b)
    {
        if (mSink)
            mSink(Gesture{t, pos, start, b});
    }

    void GestureRecognizer::feed(const RawPointer &e)
    {
        using K = RawPointer::Kind;
        switch (e.kind)
        {
        case K::Down:
            mPressed = true;
            mDragging = false;
            mDownPos = e.pos;
            mDownButton = e.button;
            emit(Gesture::Type::Down, e.pos, e.pos, e.button);
            break;

        case K::Move:
            if (mPressed && !mDragging && dist(e.pos, mDownPos) > mDragThreshold)
            {
                mDragging = true;
                emit(Gesture::Type::DragStart, e.pos, mDownPos, mDownButton);
            }
            if (mDragging)
                emit(Gesture::Type::Drag, e.pos, mDownPos, mDownButton);
            else
                emit(Gesture::Type::Move, e.pos, mPressed ? mDownPos : e.pos, mDownButton);
            break;

        case K::Up:
            emit(Gesture::Type::Up, e.pos, mDownPos, e.button);
            if (mDragging)
            {
                emit(Gesture::Type::Drop, e.pos, mDownPos, mDownButton);
            }
            else if (mDownButton == PointerButton::Right)
            {
                emit(Gesture::Type::RightClick, e.pos, e.pos, PointerButton::Right);
            }
            else
            {
                bool dbl = (e.timeMs - mLastClickTime) <= mDoubleClickMs &&
                           dist(e.pos, mLastClickPos) <= mDragThreshold;
                if (dbl)
                {
                    emit(Gesture::Type::DoubleClick, e.pos, e.pos, PointerButton::Left);
                    mLastClickTime = -1e30; // so a third tap starts fresh
                }
                else
                {
                    emit(Gesture::Type::Click, e.pos, e.pos, PointerButton::Left);
                    mLastClickTime = e.timeMs;
                    mLastClickPos = e.pos;
                }
            }
            mPressed = false;
            mDragging = false;
            break;
        }
    }
}
