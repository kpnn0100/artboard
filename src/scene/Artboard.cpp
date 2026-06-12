#include "Artboard.h"

namespace artboard
{
    void Artboard::render(IRenderTarget &t, double nowMs)
    {
        if (mOnFrame)
            mOnFrame(nowMs);

        if (mHasBackground)
        {
            t.save();
            t.setTransform(Transform::identity());
            t.beginPath();
            t.moveTo(0, 0);
            t.lineTo(mSize.w, 0);
            t.lineTo(mSize.w, mSize.h);
            t.lineTo(0, mSize.h);
            t.closePath();
            t.setFill(mBackground);
            t.fillPath();
            t.restore();
        }

        for (const auto &child : mChildren)
            child->render(t);
    }
}
