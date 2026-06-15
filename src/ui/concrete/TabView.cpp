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
            mPages[i]->y.set(tabHeight + 6.0);
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
        for (int i = 0; i < n; ++i)
        {
            const BoxStyle &bs = (i == mSelected) ? mStyle.tabActive : mStyle.tabIdle;
            drawRoundedRect(t, Rect{i * tw, 0, tw - 2.0, tabHeight}, bs.cornerRadius, bs.paint);
            t.setFill(mStyle.label.color);
            t.drawText(mTitles[i], i * tw + 10.0, tabHeight * 0.5 + mStyle.label.sizePx * 0.35, mStyle.label.sizePx);
        }
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
