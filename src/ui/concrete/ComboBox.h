/*
 *  Arstro Artboard — ComboBox: a drop-down selector, onChange(index).
 */
#pragma once
#include "../base/Segment.h"
#include "../base/Theme.h"
#include "../../anim/Animation.h"
#include "../../anim/Spring.h"
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
        /** The tallest the option list may be (FR-48). Beyond it the list scrolls rather than
         *  running off the window: thirteen easings do not fit under a control near the bottom. */
        double maxPopupHeight = 240.0;

        void setOptions(std::vector<std::string> options);
        const std::vector<std::string> &options() const { return mOptions; }
        int selectedIndex() const { return mSelected; }
        void setSelectedIndex(int index);
        bool isOpen() const { return mOpen; }
        /** Where the open list sits and how much of it is showing (FR-48). One computation, read
         *  by the hit test, the row the pointer resolves to, and the drawing — so a click always
         *  lands on the row under the cursor. */
        struct Popup
        {
            double top = 0.0;        // local y of the list's top edge (negative when it opens up)
            double height = 0.0;     // what is showing, never more than maxPopupHeight
            double content = 0.0;    // every row, laid end to end
            double maxScroll = 0.0;  // max(0, content - height)
            bool up = false;         // opened upward for want of room below
        };
        Popup popup() const;
        bool popupScrollable() const { return popup().maxScroll > 0.0; }
        double popupScroll() const { return mScroll.value(); }

        void setStyle(const ComboStyle &style) { mStyle = style; }

        /** Ticks the open/close reveal animation (fade + slide). */
        void advance(double nowMs) override;

    protected:
        void onPaint(IRenderTarget &t) const override;
        void onOverlay(IRenderTarget &t) const override;  // dropdown, drawn on top
        bool hitTestSelf(const Point &localPoint) const override;
        bool handleGesture(const Gesture &g, const Point &localPoint) override;

    private:
        /** `s`, or the longest prefix plus an ellipsis that fits `maxW` (FR-39). */
        std::string fitText(IRenderTarget &t, const std::string &s, double maxW) const;
        void setOpen(bool open); // flip logical state + start the reveal/close animation
        /** The row at a local point, or -1 when the point is not on a row. */
        int rowAt(const Point &localPoint) const;
        void scrollBy(double dy);
        ComboStyle mStyle;
        std::vector<std::string> mOptions;
        int mSelected = 0;
        bool mOpen = false;              // logical state (drives hit-testing immediately)
        AnimatedProperty mOpenAnim;      // visual reveal 0..1 (fade + slide)
        double mNowMs = 0.0;             // last advance() time, stamped for open/close
        int mHoverRow = -1;              // option row under the pointer (-1 = none)
        Spring mRowHiY{0.0};             // gliding row-highlight Y (follows the hovered row)
        Spring mRowHiA{0.0};             // row-highlight alpha (eases in/out)
        Spring mScroll{0.0};             // list scroll offset, eased (FR-48)
    };
}
