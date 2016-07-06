//
//  AudioReverb.h
//  libraries/audio/src
//
//  Created by Ken Cooke on 10/11/15.
//  Copyright 2015 High Fidelity, Inc.
//

#ifndef hifi_AudioReverb_h
#define hifi_AudioReverb_h

#include <stdint.h>

typedef struct ReverbParameters {

    float sampleRate;       // [24000, 48000] Hz
    float bandwidth;        // [20, 24000] Hz

    float preDelay;         // [0, 333] ms
    float lateDelay;        // [0, 166] ms

    float reverbTime;       // [0.1, 100] seconds

    float earlyDiffusion;   // [0, 100] percent
    float lateDiffusion;    // [0, 100] percent

    float roomSize;         // [0, 100] percent
    float density;          // [0, 100] percent

    float bassMult;         // [0.1, 10] ratio
    float bassFreq;         // [10, 500] Hz
    float highGain;         // [-24, 0] dB
    float highFreq;         // [1000, 12000] Hz

    float modRate;          // [0.1, 10] Hz
    float modDepth;         // [0, 100] percent

    float earlyGain;        // [-96, +24] dB
    float lateGain;         // [-96, +24] dB

    float earlyMixLeft;     // [0, 100] percent
    float earlyMixRight;    // [0, 100] percent
    float lateMixLeft;      // [0, 100] percent
    float lateMixRight;     // [0, 100] percent

    float wetDryMix;        // [0, 100] percent

} ReverbParameters;

class ReverbImpl;

class AudioReverb {
public:
    AudioReverb(float sampleRate);
    ~AudioReverb();

    void setParameters(ReverbParameters *p);
    void getParameters(ReverbParameters *p);
    void reset();

    // deinterleaved float input/output (native format)
    void render(float** inputs, float** outputs, int numFrames);

    // interleaved int16_t input/output
    void render(const int16_t* input, int16_t* output, int numFrames);

    // interleaved float input/output
    void render(const float* input, float* output, int numFrames);

private:
    ReverbImpl *_impl;
    ReverbParameters _params;

    float* _inout[2];

    void convertInput(const int16_t* input, float** outputs, int numFrames);
    void convertOutput(float** inputs, int16_t* output, int numFrames);

    void convertInput(const float* input, float** outputs, int numFrames);
    void convertOutput(float** inputs, float* output, int numFrames);
};

#endif // hifi_AudioReverb_h
