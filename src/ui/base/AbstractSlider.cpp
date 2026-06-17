#include "AbstractSlider.h"

namespace artboard
{
    double AbstractSlider::clamp(double value) const
    {
        if (mMax < mMin)
            return mMin;
        if (value < mMin)
            return mMin;
        if (value > mMax)
            return mMax;
        return value;
    }

    void AbstractSlider::setRange(double minimum, double maximum)
    {
        mMin = minimum;
        mMax = maximum < minimum ? minimum : maximum;
        setValue(mValue);
        mDefault = clamp(mDefault); // keep the snap-back target inside the new range
    }

    void AbstractSlider::setValue(double value)
    {
        mValue = clamp(value);
    }

    double AbstractSlider::normalizedValue() const
    {
        const double span = mMax - mMin;
        if (span <= 0.0)
            return 0.0;
        return (mValue - mMin) / span;
    }
}
