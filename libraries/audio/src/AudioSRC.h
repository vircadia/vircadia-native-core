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

#include "stdint.h"

class AudioSRC {

public:
    static const int MAX_CHANNELS = 2;

    AudioSRC(int inputSampleRate, int outputSampleRate, int numChannels);
    ~AudioSRC();

    int render(const int16_t* input, int16_t* output, int inputFrames);

    int getMinOutput(int inputFrames);
    int getMaxOutput(int inputFrames);
    int getMinInput(int outputFrames);
    int getMaxInput(int outputFrames);

private:
    float* _polyphaseFilter;
    int* _stepTable;

    float* _history[MAX_CHANNELS];
    float* _inputs[MAX_CHANNELS];
    float* _outputs[MAX_CHANNELS];

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

    int createRationalFilter(int upFactor, int downFactor, float gain);
    int createIrrationalFilter(int upFactor, int downFactor, float gain);

    int multirateFilter1(const float* input0, float* output0, int inputFrames);
    int multirateFilter2(const float* input0, const float* input1, float* output0, float* output1, int inputFrames);

    void convertInputFromInt16(const int16_t* input, float** outputs, int numFrames);
    void convertOutputToInt16(float** inputs, int16_t* output, int numFrames);

    int processFloat(float** inputs, float** outputs, int inputFrames);
};

#endif // AudioSRC_h
