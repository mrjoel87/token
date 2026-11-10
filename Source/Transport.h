#pragma once

namespace token
{
    /** Host tempo/transport snapshot, read on the UI thread from the processor's atomics. */
    struct Transport
    {
        double bpm = 120.0, ppq = 0.0, beatsPerBar = 4.0;
        bool playing = false;
    };
}
