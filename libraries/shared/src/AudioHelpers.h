//
//  AudioHelpers.h
//  libraries/shared/src
//
//  Created by Ken Cooke on 1/4/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AudioHelpers_h
#define hifi_AudioHelpers_h

#include <stdint.h>

const int IEEE754_MANT_BITS = 23;
const int IEEE754_EXPN_BIAS = 127;

//
// for x  > 0.0f, returns log2(x)
// for x <= 0.0f, returns large negative value
//
// abs |error| < 2e-4, smooth (exact for x=2^N)
// rel |error| < 0.4 from precision loss very close to 1.0f
//
static inline float fastLog2f(float x) {

    union { float f; int32_t i; } mant, bits = { x };

    // split into mantissa and exponent
    mant.i = (bits.i & ((1 << IEEE754_MANT_BITS) - 1)) | (IEEE754_EXPN_BIAS << IEEE754_MANT_BITS);
    int32_t expn = (bits.i >> IEEE754_MANT_BITS) - IEEE754_EXPN_BIAS;

    mant.f -= 1.0f;

    // polynomial for log2(1+x) over x=[0,1]
    x = (((-0.0821307180f * mant.f + 0.321188984f) * mant.f - 0.677784014f) * mant.f + 1.43872575f) * mant.f;

    return x + expn;
}

//
// for -127 <= x < 128, returns exp2(x)
// otherwise, returns undefined
//
// rel |error| < 9e-6, smooth (exact for x=N)
//
static inline float fastExp2f(float x) {

    union { float f; int32_t i; } xi;

    // bias such that x > 0
    x += IEEE754_EXPN_BIAS;

    // split into integer and fraction
    xi.i = (int32_t)x;
    x -= xi.i;

    // construct exp2(xi) as a float
    xi.i <<= IEEE754_MANT_BITS;

    // polynomial for exp2(x) over x=[0,1]
    x = (((0.0135557472f * x + 0.0520323690f) * x + 0.241379763f) * x + 0.693032121f) * x + 1.0f;

    return x * xi.f;
}

//
// Quantize a non-negative gain value to the nearest 0.5dB, and pack to a byte.
//
// Values above +30dB are clamped to +30dB
// Values below -97dB are clamped to -inf
// Value of 1.0 (+0dB) is reconstructed exactly
//
const float GAIN_CONVERSION_RATIO = 2.0f * 6.02059991f; // scale log2 to 0.5dB
const float GAIN_CONVERSION_OFFSET = 255 - 60.0f;       // translate +30dB to max

static inline uint8_t packFloatGainToByte(float gain) {

    float f = fastLog2f(gain) * GAIN_CONVERSION_RATIO + GAIN_CONVERSION_OFFSET;
    int32_t i = (int32_t)(f + 0.5f);                    // quantize
    
    uint8_t byte = (i < 0) ? 0 : ((i > 255) ? 255 : i); // clamp
    return byte;
}

static inline float unpackFloatGainFromByte(uint8_t byte) {

    float gain = (byte == 0) ? 0.0f : fastExp2f((byte - GAIN_CONVERSION_OFFSET) * (1.0f/GAIN_CONVERSION_RATIO));
    return gain;
}

#endif // hifi_AudioHelpers_h
