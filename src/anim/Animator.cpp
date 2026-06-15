#include "Animator.h"
#include <algorithm>

namespace artboard
{
    Animator::Handle Animator::tween(double from, double to, double durationMs)
    {
        auto track = std::make_shared<Track>();
        track->tween = Tween(from, to, durationMs);
        mTracks.push_back(track);
        return Handle(track);
    }

    void Animator::advance(double nowMs)
    {
        // Snapshot so callbacks may safely start new tracks during the tick;
        // newly added tracks simply wait for the next advance().
        auto snapshot = mTracks;
        std::vector<Track *> done;
        for (auto &track : snapshot)
        {
            if (!track->started)
            {
                track->started = true;
                track->start = nowMs;
            }
            double elapsed = nowMs - track->start;
            double v = track->tween.at(elapsed);
            if (track->onUpdate)
                track->onUpdate(v);
            if (track->tween.finished(elapsed))
            {
                if (track->onComplete)
                    track->onComplete();
                done.push_back(track.get());
            }
        }
        if (!done.empty())
        {
            mTracks.erase(
                std::remove_if(mTracks.begin(), mTracks.end(),
                               [&](const std::shared_ptr<Track> &t)
                               {
                                   return std::find(done.begin(), done.end(), t.get()) != done.end();
                               }),
                mTracks.end());
        }
    }
}
