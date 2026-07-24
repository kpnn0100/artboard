#include "GestureRecognizer.h"
#include <cmath>

namespace artboard
{
    static double dist(const Point &a, const Point &b)
    {
        return std::hypot(a.x - b.x, a.y - b.y);
    }

    void GestureRecognizer::emit(Gesture::Type t, const Point &pos, const Point &start, PointerButton b,
                                  const Point &velocity)
    {
        if (mSink)
        {
            Gesture g{t, pos, start, b};
            g.alt = mAlt;  // carry the current event's modifier state onto the gesture
            g.shift = mShift;
            g.ctrl = mCtrl;
            g.touch = mTouch;
            g.velocity = velocity;
            mSink(g);
        }
    }

    void GestureRecognizer::feed(const RawPointer &e)
    {
        using K = RawPointer::Kind;
        mAlt = e.alt;
        mShift = e.shift;
        mCtrl = e.ctrl;
        mTouch = e.touch;
        switch (e.kind)
        {
        case K::Down:
            mPressed = true;
            mDragging = false;
            mDownPos = e.pos;
            mDownButton = e.button;
            mDownTimeMs = e.timeMs;
            mLongPressFired = false;
            mVelocitySamples.clear();
            emit(Gesture::Type::Down, e.pos, e.pos, e.button);
            break;

        case K::Move:
            if (mPressed && !mDragging && dist(e.pos, mDownPos) > mDragThreshold)
            {
                mDragging = true;
                emit(Gesture::Type::DragStart, e.pos, mDownPos, mDownButton);
            }
            if (mDragging)
            {
                emit(Gesture::Type::Drag, e.pos, mDownPos, mDownButton);
                // Track a short rolling window of recent samples for a fling velocity at release.
                mVelocitySamples.push_back({e.timeMs, e.pos});
                const double cutoff = e.timeMs - mVelocityWindowMs;
                while (!mVelocitySamples.empty() && mVelocitySamples.front().timeMs < cutoff)
                    mVelocitySamples.erase(mVelocitySamples.begin());
            }
            else
                emit(Gesture::Type::Move, e.pos, mPressed ? mDownPos : e.pos, mDownButton);
            break;

        case K::Up:
        {
            emit(Gesture::Type::Up, e.pos, mDownPos, e.button);
            if (mDragging)
            {
                emit(Gesture::Type::Drop, e.pos, mDownPos, mDownButton);
                if (!mVelocitySamples.empty())
                {
                    const VelocitySample &oldest = mVelocitySamples.front();
                    const double dtSec = (e.timeMs - oldest.timeMs) / 1000.0;
                    if (dtSec > 0.0)
                    {
                        const Point v{(e.pos.x - oldest.pos.x) / dtSec, (e.pos.y - oldest.pos.y) / dtSec};
                        if (dist(v, Point{}) > mFlingThreshold)
                            emit(Gesture::Type::Fling, e.pos, mDownPos, mDownButton, v);
                    }
                }
            }
            else if (mDownButton == PointerButton::Right)
            {
                emit(Gesture::Type::RightClick, e.pos, e.pos, PointerButton::Right);
            }
            else if (!mLongPressFired)
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

    void GestureRecognizer::advance(double nowMs)
    {
        if (mPressed && !mDragging && !mLongPressFired && (nowMs - mDownTimeMs) >= mLongPressMs)
        {
            mLongPressFired = true;
            emit(Gesture::Type::LongPress, mDownPos, mDownPos, mDownButton);
        }
    }
}
