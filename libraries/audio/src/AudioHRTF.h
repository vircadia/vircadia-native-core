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
#include <string.h>
#include <algorithm>

#include "AudioHelpers.h"

static const int HRTF_AZIMUTHS = 72;    // 360 / 5-degree steps
static const int HRTF_TAPS = 64;        // minimum-phase FIR coefficients
static const int HRTF_TABLES = 25;      // number of HRTF subjects

static const int HRTF_DELAY = 24;       // max ITD in samples (1.0ms at 24KHz)
static const int HRTF_BLOCK = 240;      // block processing size

static const float HRTF_GAIN = 1.0f;    // HRTF global gain adjustment

// Near-field HRTF
static const float HRTF_AZIMUTH_REF = 2.0f;     // IRCAM Listen HRTF was recorded at 2 meters
static const float HRTF_NEARFIELD_MAX = 1.0f;   // distance in meters
static const float HRTF_NEARFIELD_MIN = 0.125f; // distance in meters
static const float HRTF_HEAD_RADIUS = 0.0875f;  // average human head in meters

// Distance attenuation
static const float ATTN_DISTANCE_REF = 2.0f;    // distance where attn is 0dB
static const float ATTN_GAIN_MAX = 16.0f;       // max gain allowed by distance attn (+24dB)

// Distance filter
static const float LPF_DISTANCE_REF = 256.0f;   // approximation of sound propogation in air

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
    // lpfDistance: distance filter adjustment (distance to 1kHz lowpass in meters)
    //
    void render(int16_t* input, float* output, int index, float azimuth, float distance, float gain, int numFrames,
                float lpfDistance = LPF_DISTANCE_REF);

    //
    // Non-spatialized direct mix (accumulates into existing output)
    //
    void mixMono(int16_t* input, float* output, float gain, int numFrames);
    void mixStereo(int16_t* input, float* output, float gain, int numFrames);

    //
    // Fast path when input is known to be silent and state as been flushed
    //
    void setParameterHistory(float azimuth, float distance, float gain, float lpfDistance = LPF_DISTANCE_REF) {
        // new parameters become old
        _azimuthState = azimuth;
        _distanceState = distance;
        _gainState = gain;

        _lpfState = 0.5f * fastLog2f(std::max(distance, 1.0f)) / fastLog2f(std::max(lpfDistance, 2.0f));
        _lpfState = std::min(std::max(_lpfState, 0.0f), 1.0f);
    }

    //
    // HRTF local gain adjustment in amplitude (1.0 == unity)
    //
    void setGainAdjustment(float gain) { _gainAdjust = HRTF_GAIN * gain; };
    float getGainAdjustment() { return _gainAdjust; }

    // clear internal state, but retain settings
    void reset() {
        if (!_resetState) {
            // FIR history
            memset(_firState, 0, sizeof(_firState));

            // integer delay history
            memset(_delayState, 0, sizeof(_delayState));

            // biquad history
            memset(_bqState, 0, sizeof(_bqState));

            // parameter history
            _azimuthState = 0.0f;
            _distanceState = 0.0f;
            _gainState = 0.0f;
            _lpfState = 0.0f;

            // _gainAdjust is retained

            _resetState = true;
        }
    }

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
    float _lpfState = 0.0f;

    // global and local gain adjustment
    float _gainAdjust = HRTF_GAIN;

    bool _resetState = true;
};

#endif // AudioHRTF_h
