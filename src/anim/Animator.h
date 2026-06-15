/*
 *  Arstro Artboard — Animator: a callback-based animation timeline.
 *
 *  The ergonomic "animate anything" entry point. Instead of binding a tween to a
 *  Segment property, you give it an onUpdate(value) callback and it drives that
 *  value over time. Declare a whole animation in one expression:
 *
 *      animator.tween(0, 1, 300)
 *              .easing(Easing::EaseOutBack)
 *              .onUpdate([&](double v){ glow = v; });
 *
 *  Tick every active animation once per frame with advance(nowMs); finished
 *  tracks are removed automatically. Pure timing + dispatch — no backend, no UI
 *  coupling (depends only on Tween and std::function).
 */
#pragma once
#include "Animation.h"
#include <functional>
#include <memory>
#include <vector>

namespace artboard
{
    class Animator
    {
    public:
        /** A single running animation: a Tween + its callbacks + lazy start time. */
        struct Track
        {
            Tween tween;
            std::function<void(double)> onUpdate;
            std::function<void()> onComplete;
            double start = 0.0;
            bool started = false;
        };

        /** Chainable configurator returned by tween(); mutates the underlying Track. */
        class Handle
        {
        public:
            explicit Handle(std::shared_ptr<Track> t) : mTrack(std::move(t)) {}
            Handle &easing(Easing e) { mTrack->tween.easing = e; return *this; }
            Handle &delay(double ms) { mTrack->tween.delayMs = ms; return *this; }
            Handle &repeat(int n) { mTrack->tween.repeat = n; return *this; }
            Handle &loop() { mTrack->tween.repeat = -1; return *this; }
            Handle &yoyo(bool y = true) { mTrack->tween.yoyo = y; return *this; }
            Handle &onUpdate(std::function<void(double)> cb) { mTrack->onUpdate = std::move(cb); return *this; }
            Handle &onComplete(std::function<void()> cb) { mTrack->onComplete = std::move(cb); return *this; }

        private:
            std::shared_ptr<Track> mTrack;
        };

        /** Start a from->to animation over durationMs; configure via the returned Handle. */
        Handle tween(double from, double to, double durationMs);

        /** Tick every track to time nowMs: invoke onUpdate, fire onComplete, drop finished. */
        void advance(double nowMs);

        /** Cancel all tracks immediately (no onComplete). */
        void clear() { mTracks.clear(); }

        /** Number of live tracks. */
        int activeCount() const { return static_cast<int>(mTracks.size()); }

    private:
        std::vector<std::shared_ptr<Track>> mTracks;
    };
}
