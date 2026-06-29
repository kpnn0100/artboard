/*
 *  Arstro Artboard — ComboBox: a drop-down selector, onChange(index).
 */
#pragma once
#include "../base/Segment.h"
#include "../base/Theme.h"
#include <functional>
#include <string>
#include <vector>

namespace artboard
{
    class ComboBox : public Segment
    {
    public:
        explicit ComboBox(const ComboStyle &style = Theme::basicTheme().combo);

        std::function<void(int)> onChange;
        double rowHeight = 28.0;

        void setOptions(std::vector<std::string> options);
        const std::vector<std::string> &options() const { return mOptions; }
        int selectedIndex() const { return mSelected; }
        void setSelectedIndex(int index);
        bool isOpen() const { return mOpen; }
        void setStyle(const ComboStyle &style) { mStyle = style; }

    protected:
        void onPaint(IRenderTarget &t) const override;
        void onOverlay(IRenderTarget &t) const override;  // dropdown, drawn on top
        bool hitTestSelf(const Point &localPoint) const override;
        bool handleGesture(const Gesture &g, const Point &localPoint) override;

    private:
        ComboStyle mStyle;
        std::vector<std::string> mOptions;
        int mSelected = 0;
        bool mOpen = false;
    };
}
