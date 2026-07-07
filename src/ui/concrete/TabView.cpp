#include "TabView.h"
#include "../base/Interaction.h"

namespace artboard
{
    TabView::TabView(const TabStyle &style) : mStyle(style)
    {
        width.set(300.0);
        height.set(200.0);
    }

    void TabView::addPage(const std::string &title, std::shared_ptr<Segment> page)
    {
        mTitles.push_back(title);
        mPages.push_back(page);
        mTabFade.emplace_back((int)mTitles.size() - 1 == mSelected ? 1.0 : 0.0);
        mTabHover.emplace_back(0.0);
        addChild(page);
        syncPages();
    }

    void TabView::setSelectedIndex(int index)
    {
        if (index >= 0 && index < (int)mTitles.size())
        {
            mSelected = index;
            // Ease every tab toward its new active state (the transition never snaps).
            for (int i = 0; i < (int)mTabFade.size(); ++i)
                mTabFade[i].animateTo(i == mSelected ? 1.0 : 0.0, 180.0, Easing::EaseOutCubic, mNowMs);
            syncPages();
        }
    }

    void TabView::advance(double nowMs)
    {
        const double dt = mLastMs < 0.0 ? 0.0 : (nowMs - mLastMs) / 1000.0;
        mLastMs = nowMs;
        mNowMs = nowMs;
        for (auto &f : mTabFade)
            f.update(nowMs);
        const int hov = isHovered() ? mHoverTab : -1;  // clear hover when the pointer leaves
        for (int i = 0; i < (int)mTabHover.size(); ++i)
        {
            mTabHover[i].setTarget(i == hov ? 1.0 : 0.0);
            mTabHover[i].advance(dt);
        }
        Segment::advance(nowMs);
    }

    void TabView::syncPages() const
    {
        for (int i = 0; i < (int)mPages.size(); ++i)
        {
            mPages[i]->x.set(0.0);
            mPages[i]->y.set(tabHeight);  // touches the tab strip (no gap) so the active tab unites with it
            mPages[i]->visible = (i == mSelected);
        }
    }

    void TabView::render(IRenderTarget &t, const Transform &parent) const
    {
        syncPages();
        Segment::render(t, parent);
    }

    void TabView::onPaint(IRenderTarget &t) const
    {
        const int n = (int)mTitles.size();
        if (n == 0)
            return;
        const double tw = width.value() / n;
        // Inactive tabs are recessed (start a few px down, shorter). The active tab
        // is full height and extends DOWN past the strip; the page (drawn on top of
        // this onPaint) covers the overhang, so the active tab reads as merged with
        // the content below — a united, connected-tab look. The active/idle geometry
        // and colour are interpolated by the per-tab fade factor `f` so selecting a
        // tab eases (never snaps); hover brightens the tab under the pointer.
        auto tabFade = [&](int i) { return i < (int)mTabFade.size() ? mTabFade[i].value() : (i == mSelected ? 1.0 : 0.0); };
        auto tabHover = [&](int i) { return i < (int)mTabHover.size() ? mTabHover[i].value() : 0.0; };
        auto drawTab = [&](int i) {
            const double f = tabFade(i);
            const double y = 4.0 * (1.0 - f);          // 4 (idle) -> 0 (active)
            const double hh = (tabHeight - 4.0) + f * 14.0;  // shorter (idle) -> taller (active)
            BoxStyle bs = lerpBox(mStyle.tabIdle, mStyle.tabActive, f);
            bs = hoverBox(bs, mStyle.tabActive.paint.fill, tabHover(i));
            drawRoundedRect(t, Rect{i * tw + 1.0, y, tw - 2.0, hh}, bs.cornerRadius, bs.paint);
            if (f > 0.0 && mStyle.activeIndicatorHeight > 0.0)
            {
                Color ic = mStyle.activeIndicatorColor;
                ic.a *= f;
                drawRoundedRect(t, Rect{i * tw + 1.0, 0.0, tw - 2.0, mStyle.activeIndicatorHeight}, 0.0,
                                Paint::filled(ic));
            }
            t.setFill(lerpColor(mStyle.label.color, mStyle.labelActive.color, f));
            t.drawText(mTitles[i], i * tw + 10.0, tabHeight * 0.5 + mStyle.label.sizePx * 0.35, mStyle.label.sizePx,
                       mStyle.label.fontFamily, mStyle.label.letterSpacingPx);
        };
        // Draw least-active first so the most-active (selected) tab lands on top.
        for (int i = 0; i < n; ++i)
            if (i != mSelected)
                drawTab(i);
        if (mSelected >= 0 && mSelected < n)
            drawTab(mSelected);
    }

    bool TabView::handleGesture(const Gesture &g, const Point &localPoint)
    {
        const int n = (int)mTitles.size();
        if (g.type == Gesture::Type::Move)
        {
            const bool onStrip = n > 0 && localPoint.y <= tabHeight &&
                                 localPoint.x >= 0.0 && localPoint.x <= width.value();
            mHoverTab = onStrip ? (int)(localPoint.x / (width.value() / n)) : -1;
            return Segment::handleGesture(g, localPoint);
        }
        if (g.type == Gesture::Type::Click && n > 0 && localPoint.y <= tabHeight)
        {
            const double tw = width.value() / n;
            const int i = (int)(localPoint.x / tw);
            if (i >= 0 && i < n)
            {
                setSelectedIndex(i);
                if (onChange)
                    onChange(i);
            }
            return true;
        }
        return Segment::handleGesture(g, localPoint);
    }
}
