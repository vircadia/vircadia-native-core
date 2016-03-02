//
//  AudioLimiter.h
//  libraries/audio/src
//
//  Created by Ken Cooke on 2/11/15.
//  Copyright 2016 High Fidelity, Inc.
//

#ifndef hifi_AudioLimiter_h
#define hifi_AudioLimiter_h

#include "stdint.h"

class LimiterImpl;

class AudioLimiter {
public:
    AudioLimiter(int sampleRate, int numChannels);
    ~AudioLimiter();

    void render(float* input, int16_t* output, int numFrames);

    void setThreshold(float threshold);
    void setRelease(float release);

private:
    LimiterImpl* _impl;
};

#endif // hifi_AudioLimiter_h
