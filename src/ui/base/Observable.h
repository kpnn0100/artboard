/*
 *  Arstro Artboard — Observable<T>: a single-source-of-truth value with change
 *  notification (FR-23). Several UI nodes bind to ONE Observable — a toggle
 *  button's `active`, the panel it shows/hides, a status pill — by registering
 *  an observer; flipping the value updates every observer in one place, so the
 *  views can never drift out of sync. This is the state-link primitive controls
 *  use INSTEAD of each caching its own copy of a shared boolean/enum and hoping
 *  the copies stay equal (the class of bug where a toggle button's highlight and
 *  the panel it controls disagree).
 *
 *  Platform-free, header-only (a value + a list of callbacks): no drawing, no
 *  HAL, no OS. It sits beside Property (an animated scalar) and ModBus (a
 *  modulation-value bus) as the third small state primitive in ui/base.
 */
#pragma once
#include <functional>
#include <utility>
#include <vector>

namespace artboard
{
    template <class T>
    class Observable
    {
    public:
        using Observer = std::function<void(const T &)>;

        Observable() = default;
        explicit Observable(T value) : mValue(std::move(value)) {}

        const T &get() const { return mValue; }

        /** Set the value; notify every observer only if it actually changed.
         *  Setting the current value is a no-op — so two observers that write
         *  back into the same Observable can't recurse forever. */
        void set(T value)
        {
            if (value == mValue)
                return;
            mValue = std::move(value);
            for (const auto &o : mObservers)  // observe() rejects null, so every entry is callable
                o(mValue);
        }

        /** Register an observer. By default it fires immediately with the current
         *  value so the view initializes IN SYNC (the fix for start-up drift);
         *  pass fireNow=false to only receive later changes. A null observer is
         *  ignored. */
        void observe(Observer observer, bool fireNow = true)
        {
            if (!observer)
                return;
            if (fireNow)
                observer(mValue);
            mObservers.push_back(std::move(observer));
        }

        int observerCount() const { return (int)mObservers.size(); }

    private:
        T mValue{};
        std::vector<Observer> mObservers;
    };
}
