#pragma once
#include "Segment.h"
#include "Theme.h"
#include <functional>

namespace artboard
{
    class RectangleSegment : public Segment
    {
    public:
        BoxStyle style;

    protected:
        void onPaint(IRenderTarget &t) const override;
    };

    class CircleSegment : public Segment
    {
    public:
        BoxStyle style;

    protected:
        void onPaint(IRenderTarget &t) const override;
    };

    class LabelSegment : public Segment
    {
    public:
        std::string text;
        TextStyle style;

    protected:
        void onPaint(IRenderTarget &t) const override;
        bool hitTestSelf(const Point &) const override { return false; }
    };

    enum class DragType { Absolute, Relative };

    class AbstractSlider
    {
    public:
        explicit AbstractSlider(double value = 0.0, double minimum = 0.0, double maximum = 1.0)
            : mValue(value), mMin(minimum), mMax(maximum)
        {
            setValue(value);
        }

        double value() const { return mValue; }
        double minimum() const { return mMin; }
        double maximum() const { return mMax; }
        bool isAnalog() const { return mIsAnalog; }
        DragType dragType() const { return mDragType; }

        void setRange(double minimum, double maximum);
        void setValue(double value);
        void setAnalog(bool analog) { mIsAnalog = analog; }
        void setDragType(DragType dragType) { mDragType = dragType; }

    protected:
        double normalizedValue() const;

    private:
        double clamp(double value) const;

        double mValue = 0.0;
        double mMin = 0.0;
        double mMax = 1.0;
        bool mIsAnalog = true;
        DragType mDragType = DragType::Absolute;
    };

    class Slider : public Segment, public AbstractSlider
    {
    public:
        explicit Slider(const SliderStyle &style = Theme::basicTheme().slider);

        void setStyle(const SliderStyle &style);
        const SliderStyle &style() const { return mStyle; }
        void render(IRenderTarget &t, const Transform &parent = Transform::identity()) const override;

    protected:
        bool handleGesture(const Gesture &g, const Point &localPoint) override;
        bool handleKey(const KeyEvent &event) override;

    private:
        void ensureVisualTree() const;
        void syncVisuals() const;
        double valueForLocalX(double localX) const;

        SliderStyle mStyle;
        mutable std::shared_ptr<RectangleSegment> mTrack;
        mutable std::shared_ptr<RectangleSegment> mRangeFill;
        mutable std::shared_ptr<CircleSegment> mThumb;
    };

    class Button : public Segment
    {
    public:
        explicit Button(std::string label = {}, const ButtonStyle &style = Theme::basicTheme().button);

        std::string text;
        std::function<void()> onClick;

        void setStyle(const ButtonStyle &style);
        void render(IRenderTarget &t, const Transform &parent = Transform::identity()) const override;

    protected:
        bool handleGesture(const Gesture &g, const Point &localPoint) override;
        bool handleKey(const KeyEvent &event) override;

    private:
        void ensureVisualTree() const;
        void syncVisuals() const;

        ButtonStyle mStyle;
        bool mPressed = false;
        mutable std::shared_ptr<RectangleSegment> mBody;
        mutable std::shared_ptr<LabelSegment> mLabel;
    };

    class Checkbox : public Segment
    {
    public:
        explicit Checkbox(std::string label = {}, const CheckboxStyle &style = Theme::basicTheme().checkbox);

        std::string text;

        bool checked() const { return mChecked; }
        void setChecked(bool checked) { mChecked = checked; }
        void setStyle(const CheckboxStyle &style);
        void render(IRenderTarget &t, const Transform &parent = Transform::identity()) const override;

    protected:
        bool handleGesture(const Gesture &g, const Point &localPoint) override;
        bool handleKey(const KeyEvent &event) override;

    private:
        void ensureVisualTree() const;
        void syncVisuals() const;
        void toggle();

        CheckboxStyle mStyle;
        bool mChecked = false;
        mutable std::shared_ptr<RectangleSegment> mBox;
        mutable std::shared_ptr<RectangleSegment> mIndicator;
        mutable std::shared_ptr<LabelSegment> mLabel;
    };

    class TextBox : public Segment
    {
    public:
        explicit TextBox(const TextBoxStyle &style = Theme::basicTheme().textBox);

        std::string text;
        std::string placeholder;
        bool readOnly = false;

        void setStyle(const TextBoxStyle &style);
        void render(IRenderTarget &t, const Transform &parent = Transform::identity()) const override;

    protected:
        bool handleGesture(const Gesture &g, const Point &localPoint) override;
        bool handleKey(const KeyEvent &event) override;

    private:
        void ensureVisualTree() const;
        void syncVisuals() const;
        double estimateTextWidth(const std::string &value, double sizePx) const;

        TextBoxStyle mStyle;
        mutable std::shared_ptr<RectangleSegment> mBox;
        mutable std::shared_ptr<LabelSegment> mLabel;
        mutable std::shared_ptr<RectangleSegment> mCaret;
    };
}