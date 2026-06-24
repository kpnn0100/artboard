#include "TabView.h"

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
        addChild(page);
        syncPages();
    }

    void TabView::setSelectedIndex(int index)
    {
        if (index >= 0 && index < (int)mTitles.size())
        {
            mSelected = index;
            syncPages();
        }
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
        // the content below — a united, connected-tab look.
        auto drawTab = [&](int i, bool active) {
            const BoxStyle &bs = active ? mStyle.tabActive : mStyle.tabIdle;
            const double y = active ? 0.0 : 4.0;
            const double hh = active ? tabHeight + 10.0 : tabHeight - 4.0;
            drawRoundedRect(t, Rect{i * tw + 1.0, y, tw - 2.0, hh}, bs.cornerRadius, bs.paint);
            t.setFill(mStyle.label.color);
            t.drawText(mTitles[i], i * tw + 10.0, tabHeight * 0.5 + mStyle.label.sizePx * 0.35, mStyle.label.sizePx);
        };
        for (int i = 0; i < n; ++i)
            if (i != mSelected)
                drawTab(i, false);
        if (mSelected >= 0 && mSelected < n)
            drawTab(mSelected, true);  // active last, on top
    }

    bool TabView::handleGesture(const Gesture &g, const Point &localPoint)
    {
        const int n = (int)mTitles.size();
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
