#include "ComboBox.h"
#include "../base/Interaction.h"

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

    void ComboBox::advance(double nowMs)
    {
        const double dt = mNowMs <= 0.0 ? 0.0 : (nowMs - mNowMs) / 1000.0;
        mNowMs = nowMs;
        mOpenAnim.update(nowMs);

        // Glide a highlight bar to the hovered option row; fade it in only while a row
        // is actually hovered in the open list (so it never pops between rows).
        const bool showHi = mOpen && isHovered() && mHoverRow >= 0 && mHoverRow < (int)mOptions.size();
        if (showHi)
        {
            const double targetY = height.value() + mHoverRow * rowHeight;
            if (mRowHiA.value() < 0.01)
                mRowHiY.reset(targetY);  // appear at the row, don't glide up from the top
            mRowHiY.setTarget(targetY);
        }
        mRowHiA.setTarget(showHi ? 1.0 : 0.0);
        mRowHiY.advance(dt);
        mRowHiA.advance(dt);
        Segment::advance(nowMs);
    }

    void ComboBox::setOpen(bool open)
    {
        mOpen = open; // logical state flips at once so rows stay hit-testable while revealing
        mOpenAnim.animateTo(open ? 1.0 : 0.0, 160.0, Easing::EaseOutCubic, mNowMs);
    }

    bool ComboBox::handleGesture(const Gesture &g, const Point &localPoint)
    {
        if (g.type == Gesture::Type::Move)
        {
            // Track which open-list row the pointer is over (drives the hover highlight).
            const double h = height.value();
            mHoverRow = (mOpen && localPoint.y > h)
                            ? (int)((localPoint.y - h) / rowHeight)
                            : -1;
            return Segment::handleGesture(g, localPoint);
        }
        if (g.type != Gesture::Type::Click)
            return Segment::handleGesture(g, localPoint);

        const double h = height.value();
        if (!mOpen)
        {
            setOpen(true);
            raise();  // hit-tested first so dropdown clicks don't fall through to siblings
            return true;
        }
        if (localPoint.y <= h)
        {
            setOpen(false);
            return true;
        }
        const int row = (int)((localPoint.y - h) / rowHeight);
        if (row >= 0 && row < (int)mOptions.size())
        {
            mSelected = row;
            if (onChange)
                onChange(row);
        }
        setOpen(false);
        return true;
    }

    void ComboBox::onPaint(IRenderTarget &t) const
    {
        const double w = width.value(), h = height.value();
        // Hover: brighten the field, pull its border toward the accent (caret colour).
        const double dim = disabledAmount();
        const BoxStyle field = dimBox(hoverBox(mStyle.field, mStyle.caretColor, hoverAmount()), dim);
        drawRoundedRect(t, Rect{0, 0, w, h}, field.cornerRadius, field.paint);

        t.setFill(dimColor(mStyle.text.color, dim));
        const std::string sel = mOptions.empty() ? std::string() : mOptions[mSelected];
        // FR-39: shorten to fit between the left padding and the caret triangle, measured
        // with the adapter's own metrics — a long option name never runs under the caret.
        t.drawText(fitText(t, sel, w - 10.0 - 22.0), 10.0, h * 0.5 + mStyle.text.sizePx * 0.35,
                   mStyle.text.sizePx, mStyle.text.fontFamily, mStyle.text.letterSpacingPx);

        // caret triangle
        t.beginPath();
        t.moveTo(w - 18.0, h * 0.5 - 3.0);
        t.lineTo(w - 10.0, h * 0.5 - 3.0);
        t.lineTo(w - 14.0, h * 0.5 + 3.0);
        t.closePath();
        t.setFill(dimColor(mStyle.caretColor, dim));
        t.fillPath();
    }

    std::string ComboBox::fitText(IRenderTarget &t, const std::string &s, double maxW) const
    {
        auto width = [&](const std::string &v) {
            return t.measureText(v, mStyle.text.sizePx, mStyle.text.fontFamily,
                                 mStyle.text.letterSpacingPx);
        };
        if (maxW <= 0.0)
            return std::string();   // no room at all: draw nothing rather than overflow
        if (width(s) <= maxW)
            return s;
        const std::string dots = "\u2026";
        if (width(dots) > maxW)
            return std::string();
        std::string cut = s;
        while (!cut.empty())
        {
            do
                cut.pop_back();
            while (!cut.empty() && ((unsigned char)cut.back() & 0xC0) == 0x80);  // whole codepoints
            if (width(cut + dots) <= maxW)
                return cut + dots;
        }
        return dots;
    }

    void ComboBox::onOverlay(IRenderTarget &t) const
    {
        // Reveal progress 0..1 (fade + slide); early-out once fully closed. Logical
        // mOpen already governs hit-testing, so this is purely visual.
        const double p = mOpenAnim.value();
        if (p <= 1e-3)
            return;
        // Drawn in the overlay pass so it sits on top of every other control and is
        // never clipped by the owning panel. An opaque scrim under the popup hides
        // whatever is behind it. The list fades in and slides down into place.
        const double w = width.value(), h = height.value();
        const double popupH = (double)mOptions.size() * rowHeight;
        const double yoff = (1.0 - p) * -6.0;
        auto fade = [p](Color c) { c.a *= p; return c; };

        drawRoundedRect(t, Rect{-1, h - 1 + yoff, w + 2, popupH + 2}, mStyle.popup.cornerRadius,
                        Paint::filled(fade(mStyle.field.paint.fill)));  // opaque backing
        Paint pop = mStyle.popup.paint;
        pop.fill = fade(pop.fill);
        pop.stroke = fade(pop.stroke);
        drawRoundedRect(t, Rect{0, h + yoff, w, popupH}, mStyle.popup.cornerRadius, pop);

        // Gliding hover highlight under the row the pointer rests on (fades with mRowHiA).
        const double hiA = mRowHiA.value() * p;
        if (hiA > 0.003)
        {
            Color hc = mStyle.caretColor;
            hc.a *= 0.28 * hiA;
            drawRoundedRect(t, Rect{0, mRowHiY.value() + yoff, w, rowHeight},
                            mStyle.rowSelected.cornerRadius, Paint::filled(hc));
        }
        for (int i = 0; i < (int)mOptions.size(); ++i)
        {
            const double ry = h + yoff + i * rowHeight;
            if (i == mSelected)
            {
                Paint sel = mStyle.rowSelected.paint;
                sel.fill = fade(sel.fill);
                sel.stroke = fade(sel.stroke);
                drawRoundedRect(t, Rect{0, ry, w, rowHeight}, mStyle.rowSelected.cornerRadius, sel);
            }
            t.setFill(fade(mStyle.text.color));
            t.drawText(mOptions[i], 10.0, ry + rowHeight * 0.5 + mStyle.text.sizePx * 0.35, mStyle.text.sizePx,
                       mStyle.text.fontFamily, mStyle.text.letterSpacingPx);
        }
    }
}
