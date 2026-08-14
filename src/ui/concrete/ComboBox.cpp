#include "ComboBox.h"
#include <cmath>
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

    ComboBox::Popup ComboBox::popup() const
    {
        Popup p;
        const double h = height.value();
        p.content = (double)mOptions.size() * rowHeight;
        // Whole rows only, up to the bound: half a row reads as a rendering fault rather than as
        // "there is more", and the indicator is what says there is more (FR-48).
        const int cap = std::max(1, (int)std::floor(maxPopupHeight / std::max(1.0, rowHeight)));
        int rows = std::min((int)mOptions.size(), cap);

        // Room is measured against the ROOT — the window — because an overlay is deliberately not
        // clipped by the panel that owns it, so the panel's bounds say nothing about visibility.
        const Segment *root = this;
        while (root->parent())
            root = root->parent();
        const double worldY = worldTransform().apply(Point{0.0, 0.0}).y;
        const double rootH = root->height.value();
        const double margin = 4.0;
        double below = rootH - (worldY + h) - margin;
        double above = worldY - margin;
        if (root == this) below = above = p.content;   // unattached: no edge to respect

        double want = (double)rows * rowHeight;
        p.up = want > below && above > below;          // no room down, and more room up
        const double room = p.up ? above : below;
        if (want > room)                              // still short: show what fits, scroll the rest
            rows = std::max(1, (int)std::floor(room / std::max(1.0, rowHeight)));
        p.height = (double)rows * rowHeight;
        p.top = p.up ? -p.height : h;
        p.maxScroll = std::max(0.0, p.content - p.height);
        return p;
    }

    int ComboBox::rowAt(const Point &localPoint) const
    {
        if (!mOpen) return -1;
        const Popup p = popup();
        if (localPoint.y < p.top || localPoint.y >= p.top + p.height) return -1;
        const int row = (int)std::floor((localPoint.y - p.top + mScroll.value()) / rowHeight);
        return (row >= 0 && row < (int)mOptions.size()) ? row : -1;
    }

    void ComboBox::scrollBy(double dy)
    {
        const Popup p = popup();
        mScroll.setTarget(std::min(p.maxScroll, std::max(0.0, mScroll.target() + dy)));
    }

    bool ComboBox::hitTestSelf(const Point &localPoint) const
    {
        const double w = width.value(), h = height.value();
        if (localPoint.x < 0.0 || localPoint.x > w)
            return false;
        if (localPoint.y >= 0.0 && localPoint.y <= h)
            return true;
        if (mOpen)
        {
            const Popup p = popup();
            return localPoint.y >= p.top && localPoint.y <= p.top + p.height;
        }
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
            const double targetY = popup().top + (double)mHoverRow * rowHeight - mScroll.value();
            if (mRowHiA.value() < 0.01)
                mRowHiY.reset(targetY);  // appear at the row, don't glide up from the top
            mRowHiY.setTarget(targetY);
        }
        mRowHiA.setTarget(showHi ? 1.0 : 0.0);
        mRowHiY.advance(dt);
        mRowHiA.advance(dt);
        mScroll.advance(dt);   // the list offset eases like every other visible change (FR-48)
        Segment::advance(nowMs);
    }

    void ComboBox::setOpen(bool open)
    {
        mOpen = open; // logical state flips at once so rows stay hit-testable while revealing
        mOpenAnim.animateTo(open ? 1.0 : 0.0, 160.0, Easing::EaseOutCubic, mNowMs);
        if (open)
        {
            // Open showing the current choice, centred where it can be: otherwise picking the
            // thirteenth option means opening a list that looks like it starts at the first.
            const Popup p = popup();
            const double centred = (double)mSelected * rowHeight - (p.height - rowHeight) * 0.5;
            mScroll.reset(std::min(p.maxScroll, std::max(0.0, centred)));
        }
    }

    bool ComboBox::handleGesture(const Gesture &g, const Point &localPoint)
    {
        if (g.type == Gesture::Type::Move)
        {
            // Track which open-list row the pointer is over (drives the hover highlight).
            mHoverRow = rowAt(localPoint);
            return Segment::handleGesture(g, localPoint);
        }
        // A long list scrolls by wheel and by drag (FR-48). Returning false when there is nothing
        // out of view lets the gesture bubble to whatever can use it (FR-46).
        if (g.type == Gesture::Type::Scroll)
        {
            if (!mOpen || !popupScrollable())
                return false;
            scrollBy(g.delta.y);
            return true;
        }
        if (mOpen && (g.type == Gesture::Type::Drag || g.type == Gesture::Type::DragStart))
        {
            if (!popupScrollable())
                return false;
            scrollBy(-(localPoint.y - toLocal(g.start).y) * 0.35);
            return true;
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
        if (localPoint.y >= 0.0 && localPoint.y <= h)
        {
            setOpen(false);
            return true;
        }
        const int row = rowAt(localPoint);
        if (row >= 0)
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
        if (!drawsBuiltInVisuals)   // FR-41: the subclass draws its own appearance
            return;
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
        const double w = width.value();
        // ONE geometry, shared with the hit test: what is drawn is what is hit (FR-48). It slides
        // in from the side it opened towards, so the motion reads as "unfolding from the control".
        const Popup box = popup();
        const double yoff = (1.0 - p) * (box.up ? 6.0 : -6.0);
        const double top = box.top + yoff;
        auto fade = [p](Color c) { c.a *= p; return c; };

        drawRoundedRect(t, Rect{-1, top - 1, w + 2, box.height + 2}, mStyle.popup.cornerRadius,
                        Paint::filled(fade(mStyle.field.paint.fill)));  // opaque backing
        Paint pop = mStyle.popup.paint;
        pop.fill = fade(pop.fill);
        pop.stroke = fade(pop.stroke);
        drawRoundedRect(t, Rect{0, top, w, box.height}, mStyle.popup.cornerRadius, pop);

        // Clipped to the list: a scrolled row must not draw over the control or past the edge.
        t.save();
        t.clipRect(0.0, top, w, box.height);
        const double scroll = mScroll.value();

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
            const double ry = top + (double)i * rowHeight - scroll;
            if (ry + rowHeight < top || ry > top + box.height)
                continue;                       // scrolled out of the list: nothing to draw
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
        t.restore();

        // The indicator, drawn ONLY while something is out of view (FR-47): a bar that is always
        // there claims more content than exists.
        if (box.maxScroll > 0.0)
        {
            const double barW = 3.0;
            const double thumbH = std::max(18.0, box.height * box.height / box.content);
            const double travel = box.height - thumbH;
            const double ty = top + travel * (scroll / box.maxScroll);
            Color track = mStyle.text.color;
            track.a *= 0.10 * p;
            Color thumb = mStyle.text.color;
            thumb.a *= 0.45 * p;
            drawRoundedRect(t, Rect{w - barW - 2.0, top, barW, box.height}, barW * 0.5,
                            Paint::filled(track));
            drawRoundedRect(t, Rect{w - barW - 2.0, ty, barW, thumbH}, barW * 0.5,
                            Paint::filled(thumb));
        }
    }
}
