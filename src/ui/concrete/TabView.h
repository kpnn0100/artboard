/*
 *  Arstro Artboard — TabView: a tab strip + swappable pages, onChange(index).
 */
#pragma once
#include "../base/Segment.h"
#include "../base/Theme.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace artboard
{
    class TabView : public Segment
    {
    public:
        explicit TabView(const TabStyle &style = Theme::basicTheme().tab);

        std::function<void(int)> onChange;
        double tabHeight = 32.0;

        void addPage(const std::string &title, std::shared_ptr<Segment> page);
        int pageCount() const { return (int)mTitles.size(); }
        int selectedIndex() const { return mSelected; }
        void setSelectedIndex(int index);
        void setStyle(const TabStyle &style) { mStyle = style; }

        void render(IRenderTarget &t, const Transform &parent = Transform::identity()) const override;

    protected:
        void onPaint(IRenderTarget &t) const override;
        bool handleGesture(const Gesture &g, const Point &localPoint) override;

    private:
        void syncPages() const;
        TabStyle mStyle;
        std::vector<std::string> mTitles;
        std::vector<std::shared_ptr<Segment>> mPages;
        int mSelected = 0;
    };
}
