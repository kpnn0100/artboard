/*
 *  Arstro Artboard — AbstractSlider: ranged scalar value + drag semantics with no
 *  visual concerns. Reused by concrete ranged controls (Slider, Knob).
 */
#pragma once

namespace artboard
{
    enum class DragType { Absolute, Relative };

    class AbstractSlider
    {
    public:
        explicit AbstractSlider(double value = 0.0, double minimum = 0.0, double maximum = 1.0)
            : mValue(value), mMin(minimum), mMax(maximum)
        {
            setValue(value);
            mDefault = mValue; // default snaps back to the initial value until set otherwise
        }

        double value() const { return mValue; }
        double minimum() const { return mMin; }
        double maximum() const { return mMax; }
        double defaultValue() const { return mDefault; }
        bool isAnalog() const { return mIsAnalog; }
        DragType dragType() const { return mDragType; }

        void setRange(double minimum, double maximum);
        void setValue(double value);
        void setDefault(double value) { mDefault = clamp(value); } // snap-back target (double-click)
        void resetToDefault() { setValue(mDefault); }
        void setAnalog(bool analog) { mIsAnalog = analog; }
        void setDragType(DragType dragType) { mDragType = dragType; }

    protected:
        double normalizedValue() const;

    private:
        double clamp(double value) const;

        double mValue = 0.0;
        double mMin = 0.0;
        double mMax = 1.0;
        double mDefault = 0.0;
        bool mIsAnalog = true;
        DragType mDragType = DragType::Absolute;
    };
}
