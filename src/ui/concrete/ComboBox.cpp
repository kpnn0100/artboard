#include "ComboBox.h"

namespace artboard
{
    ComboBox::ComboBox(const ComboStyle &style) : mStyle(style)
    {
        focusable = true;
        width.set(160.0);
        height.set(32.0);
    }

    void ComboBox::setOptions(std::vector<std::string> options)
    {
        mOptions = std::move(options);
        if (mSelected >= (int)mOptions.size())
            mSelected = 0;
    }

    void ComboBox::setSelectedIndex(int index)
    {
        if (index >= 0 && index < (int)mOptions.size())
            mSelected = index;
    }

    bool ComboBox::hitTestSelf(const Point &localPoint) const
    {
        const double w = width.value(), h = height.value();
        if (localPoint.x < 0.0 || localPoint.x > w)
            return false;
        if (localPoint.y >= 0.0 && localPoint.y <= h)
            return true;
        if (mOpen)
            return localPoint.y > h && localPoint.y <= h + (double)mOptions.size() * rowHeight;
        return false;
    }

    bool ComboBox::handleGesture(const Gesture &g, const Point &localPoint)
    {
        if (g.type != Gesture::Type::Click)
            return Segment::handleGesture(g, localPoint);

        const double h = height.value();
        if (!mOpen)
        {
            mOpen = true;
            raise();  // hit-tested first so dropdown clicks don't fall through to siblings
            return true;
        }
        if (localPoint.y <= h)
        {
            mOpen = false;
            return true;
        }
        const int row = (int)((localPoint.y - h) / rowHeight);
        if (row >= 0 && row < (int)mOptions.size())
        {
            mSelected = row;
            if (onChange)
                onChange(row);
        }
        mOpen = false;
        return true;
    }

    void ComboBox::onPaint(IRenderTarget &t) const
    {
        const double w = width.value(), h = height.value();
        drawRoundedRect(t, Rect{0, 0, w, h}, mStyle.field.cornerRadius, mStyle.field.paint);

        t.setFill(mStyle.text.color);
        const std::string &sel = mOptions.empty() ? std::string() : mOptions[mSelected];
        t.drawText(sel, 10.0, h * 0.5 + mStyle.text.sizePx * 0.35, mStyle.text.sizePx);

        // caret triangle
        t.beginPath();
        t.moveTo(w - 18.0, h * 0.5 - 3.0);
        t.lineTo(w - 10.0, h * 0.5 - 3.0);
        t.lineTo(w - 14.0, h * 0.5 + 3.0);
        t.closePath();
        t.setFill(mStyle.caretColor);
        t.fillPath();
    }

    void ComboBox::onOverlay(IRenderTarget &t) const
    {
        if (!mOpen)
            return;
        // Drawn in the overlay pass so it sits on top of every other control and is
        // never clipped by the owning panel. An opaque scrim under the popup hides
        // whatever is behind it.
        const double w = width.value(), h = height.value();
        const double popupH = (double)mOptions.size() * rowHeight;
        drawRoundedRect(t, Rect{-1, h - 1, w + 2, popupH + 2}, mStyle.popup.cornerRadius,
                        Paint::filled(mStyle.field.paint.fill));  // opaque backing
        drawRoundedRect(t, Rect{0, h, w, popupH}, mStyle.popup.cornerRadius, mStyle.popup.paint);
        for (int i = 0; i < (int)mOptions.size(); ++i)
        {
            const double ry = h + i * rowHeight;
            if (i == mSelected)
                drawRoundedRect(t, Rect{0, ry, w, rowHeight}, mStyle.rowSelected.cornerRadius, mStyle.rowSelected.paint);
            t.setFill(mStyle.text.color);
            t.drawText(mOptions[i], 10.0, ry + rowHeight * 0.5 + mStyle.text.sizePx * 0.35, mStyle.text.sizePx);
        }
    }
}
