//
//  AudioSRC.h
//  libraries/audio/src
//
//  Created by Ken Cooke on 9/18/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AudioSRC_h
#define hifi_AudioSRC_h

#include <stdint.h>

static const int SRC_MAX_CHANNELS = 4;

// polyphase filter
static const int SRC_PHASEBITS = 9;
static const int SRC_PHASES = (1 << SRC_PHASEBITS);
static const int SRC_FRACBITS = 32 - SRC_PHASEBITS;
static const uint32_t SRC_FRACMASK = (1 << SRC_FRACBITS) - 1;

static const float QFRAC_TO_FLOAT = 1.0f / (1 << SRC_FRACBITS);
static const float Q32_TO_FLOAT = 1.0f / (1ULL << 32);

// blocking size in frames, chosen so block processing fits in L1 cache
static const int SRC_BLOCK = 256;

class AudioSRC {

public:
    enum Quality {
        LOW_QUALITY,
        MEDIUM_QUALITY,
        HIGH_QUALITY
    };

    AudioSRC(int inputSampleRate, int outputSampleRate, int numChannels, Quality quality = MEDIUM_QUALITY);
    ~AudioSRC();

    // deinterleaved float input/output (native format)
    int render(float** inputs, float** outputs, int inputFrames);

    // interleaved int16_t input/output
    int render(const int16_t* input, int16_t* output, int inputFrames);

    // interleaved float input/output
    int render(const float* input, float* output, int inputFrames);

    int getMinOutput(int inputFrames);
    int getMaxOutput(int inputFrames);
    int getMinInput(int outputFrames);
    int getMaxInput(int outputFrames);

private:
    float* _polyphaseFilter;
    int* _stepTable;

    float* _history[SRC_MAX_CHANNELS];
    float* _inputs[SRC_MAX_CHANNELS];
    float* _outputs[SRC_MAX_CHANNELS];

    int _inputSampleRate;
    int _outputSampleRate;
    int _numChannels;
    int _inputBlock;

    int _upFactor;
    int _downFactor;
    int _numTaps;
    int _numHistory;

    int _phase;
    int64_t _offset;
    int64_t _step;

    int createRationalFilter(int upFactor, int downFactor, float gain, Quality quality);
    int createIrrationalFilter(int upFactor, int downFactor, float gain, Quality quality);

    int multirateFilter1(const float* input0, float* output0, int inputFrames);
    int multirateFilter2(const float* input0, const float* input1, float* output0, float* output1, int inputFrames);
    int multirateFilter4(const float* input0, const float* input1, const float* input2, const float* input3, 
                         float* output0, float* output1, float* output2, float* output3, int inputFrames);

    int multirateFilter1_ref(const float* input0, float* output0, int inputFrames);
    int multirateFilter2_ref(const float* input0, const float* input1, float* output0, float* output1, int inputFrames);
    int multirateFilter4_ref(const float* input0, const float* input1, const float* input2, const float* input3, 
                             float* output0, float* output1, float* output2, float* output3, int inputFrames);

    int multirateFilter1_AVX2(const float* input0, float* output0, int inputFrames);
    int multirateFilter2_AVX2(const float* input0, const float* input1, float* output0, float* output1, int inputFrames);
    int multirateFilter4_AVX2(const float* input0, const float* input1, const float* input2, const float* input3, 
                              float* output0, float* output1, float* output2, float* output3, int inputFrames);

    void convertInput(const int16_t* input, float** outputs, int numFrames);
    void convertOutput(float** inputs, int16_t* output, int numFrames);

    void convertInput(const float* input, float** outputs, int numFrames);
    void convertOutput(float** inputs, float* output, int numFrames);
};

#endif // AudioSRC_h
