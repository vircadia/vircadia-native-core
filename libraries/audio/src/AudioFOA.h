//
//  AudioFOA.h
//  libraries/audio/src
//
//  Created by Ken Cooke on 10/28/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AudioFOA_h
#define hifi_AudioFOA_h

#include <stdint.h>

static const int FOA_TAPS = 273;    // FIR coefs
static const int FOA_NFFT = 512;    // FFT length
static const int FOA_OVERLAP = FOA_TAPS - 1;
static const int FOA_TABLES = 25;   // number of HRTF subjects

static const int FOA_BLOCK = 240;   // block processing size

static const float FOA_GAIN = 1.0f; // FOA global gain adjustment

static_assert((FOA_BLOCK + FOA_OVERLAP) == FOA_NFFT, "FFT convolution requires L+M-1 == NFFT");

class AudioFOA {

public:
    AudioFOA() {
        // identity matrix
        _rotationState[0][0] = 1.0f;
        _rotationState[1][1] = 1.0f;
        _rotationState[2][2] = 1.0f;
    };

    //
    // input: interleaved First-Order Ambisonic source
    // output: interleaved stereo mix buffer (accumulates into existing output)
    // index: HRTF subject index
    // qw, qx, qy, qz: normalized quaternion for orientation
    // gain: gain factor for volume control
    // numFrames: must be FOA_BLOCK in this version
    //
    void render(int16_t* input, float* output, int index, float qw, float qx, float qy, float qz, float gain, int numFrames);

private:
    AudioFOA(const AudioFOA&) = delete;
    AudioFOA& operator=(const AudioFOA&) = delete;

    // For best cache utilization when processing thousands of instances, only
    // the minimum persistant state is stored here. No coefs or work buffers.

    // input history, for overlap-save
    float _fftState[4][FOA_OVERLAP] = {};

    // orientation history
    float _rotationState[3][3] = {};
};

#endif // AudioFOA_h
