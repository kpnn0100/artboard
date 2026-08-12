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

        void setOptions(std::vector<std::string> options);
        const std::vector<std::string> &options() const { return mOptions; }
        int selectedIndex() const { return mSelected; }
        void setSelectedIndex(int index);
        bool isOpen() const { return mOpen; }
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
        ComboStyle mStyle;
        std::vector<std::string> mOptions;
        int mSelected = 0;
        bool mOpen = false;              // logical state (drives hit-testing immediately)
        AnimatedProperty mOpenAnim;      // visual reveal 0..1 (fade + slide)
        double mNowMs = 0.0;             // last advance() time, stamped for open/close
        int mHoverRow = -1;              // option row under the pointer (-1 = none)
        Spring mRowHiY{0.0};             // gliding row-highlight Y (follows the hovered row)
        Spring mRowHiA{0.0};             // row-highlight alpha (eases in/out)
    };
}
