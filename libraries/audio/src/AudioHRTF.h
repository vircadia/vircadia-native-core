//
//  AudioHRTF.h
//  libraries/audio/src
//
//  Created by Ken Cooke on 1/17/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AudioHRTF_h
#define hifi_AudioHRTF_h

#include <stdint.h>

static const int HRTF_AZIMUTHS = 72;    // 360 / 5-degree steps
static const int HRTF_TAPS = 64;        // minimum-phase FIR coefficients
static const int HRTF_TABLES = 25;      // number of HRTF subjects

static const int HRTF_DELAY = 24;       // max ITD in samples (1.0ms at 24KHz)
static const int HRTF_BLOCK = 240;      // block processing size

static const float HRTF_GAIN = 1.0f;    // HRTF global gain adjustment

class AudioHRTF {

public:
    AudioHRTF() {};

    //
    // input: mono source
    // output: interleaved stereo mix buffer (accumulates into existing output)
    // index: HRTF subject index
    // azimuth: clockwise panning angle in radians
    // distance: source distance in meters
    // gain: gain factor for distance attenuation
    // numFrames: must be HRTF_BLOCK in this version
    //
    void render(int16_t* input, float* output, int index, float azimuth, float distance, float gain, int numFrames);

    //
    // Fast path when input is known to be silent
    //
    void renderSilent(int16_t* input, float* output, int index, float azimuth, float distance, float gain, int numFrames);

private:
    AudioHRTF(const AudioHRTF&) = delete;
    AudioHRTF& operator=(const AudioHRTF&) = delete;

    // SIMD channel assignmentS
    enum Channel {
        L0, R0,
        L1, R1,
        L2, R2,
        L3, R3
    };

    // For best cache utilization when processing thousands of instances, only
    // the minimum persistant state is stored here. No coefs or work buffers.

    // FIR history
    float _firState[HRTF_TAPS] = {};

    // integer delay history
    float _delayState[4][HRTF_DELAY] = {};

    // biquad history
    float _bqState[3][8] = {};

    // parameter history
    float _azimuthState = 0.0f;
    float _distanceState = 0.0f;
    float _gainState = 0.0f;

    bool _silentState = false;
};

#endif // AudioHRTF_h
