/*
 *  Arstro Artboard — ModBus: the live modulation-source value bus. Modulation
 *  sources (LFOs, macros, envelopes …) publish a current value keyed by an integer
 *  id; modulation targets (a Knob with a depth ring) read it to compute and draw
 *  their modulated value. Keeping the bus separate decouples a target from any
 *  concrete source — a Knob depends only on this small abstraction, not on the app.
 *
 *  Source values are conventionally bipolar in [-1,1] (e.g. an LFO) or unipolar in
 *  [0,1] (e.g. a macro); a target scales them by a per-routing depth.
 */
#pragma once
#include <map>

namespace artboard
{
    class ModBus
    {
    public:
        void set(int sourceId, double value) { mValues[sourceId] = value; }
        double value(int sourceId) const
        {
            auto it = mValues.find(sourceId);
            return it == mValues.end() ? 0.0 : it->second;
        }
        void clear() { mValues.clear(); }

    private:
        std::map<int, double> mValues;
    };
}
