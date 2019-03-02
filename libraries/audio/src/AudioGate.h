//
//  AudioGate.h
//  libraries/audio/src
//
//  Created by Ken Cooke on 5/5/17.
//  Copyright 2016 High Fidelity, Inc.
//

#ifndef hifi_AudioGate_h
#define hifi_AudioGate_h

#include <stdint.h>

class GateImpl;

class AudioGate {
public:
    AudioGate(int sampleRate, int numChannels);
    ~AudioGate();

    //
    // Process interleaved int16_t input/output (in-place is allowed).
    // Returns true when output is non-zero.
    //
    bool render(int16_t* input, int16_t* output, int numFrames);
    bool removeDC(int16_t* input, int16_t* output, int numFrames);

    void setThreshold(float threshold);
    void setRelease(float release);

private:
    GateImpl* _impl;
};

#endif // hifi_AudioGate_h
