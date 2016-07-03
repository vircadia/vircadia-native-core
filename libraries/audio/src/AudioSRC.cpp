//
//  AudioSRC.cpp
//  libraries/audio/src
//
//  Created by Ken Cooke on 9/18/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <assert.h>
#include <string.h>
#include <algorithm>

#include "AudioSRC.h"
#include "AudioSRCData.h"

// high/low part of int64_t
#define LO32(a)   ((uint32_t)(a))
#define HI32(a)   ((int32_t)((a) >> 32))

//
// Portable aligned malloc/free
//
static void* aligned_malloc(size_t size, size_t alignment) {
    if ((alignment & (alignment-1)) == 0) {
        void* p = malloc(size + sizeof(void*) + (alignment-1));
        if (p) {
            void* ptr = (void*)(((size_t)p + sizeof(void*) + (alignment-1)) & ~(alignment-1));
            ((void**)ptr)[-1] = p;
            return ptr;
        }
    }
    return nullptr;
}

static void aligned_free(void* ptr) {
    if (ptr) {
        void* p = ((void**)ptr)[-1];
        free(p);
    }
}

// find the greatest common divisor
static int gcd(int a, int b)
{
    while (a != b) {
        if (a > b) {
            a -= b;
        } else {
            b -= a;
        }
    }
    return a;
}

//
// 3rd-order Lagrange interpolation
//
// Lagrange interpolation is maximally flat near dc and well suited
// for further upsampling our heavily-oversampled prototype filter.
//
static void cubicInterpolation(const float* input, float* output, int inputSize, int outputSize, float gain) {

    int64_t step = ((int64_t)inputSize << 32) / outputSize;     // Q32
    int64_t offset = (outputSize < inputSize) ? (step/2) : 0;   // offset to improve small integer ratios

    // Lagrange interpolation using Farrow structure
    for (int j = 0; j < outputSize; j++) {

        int32_t i = HI32(offset);
        uint32_t f = LO32(offset);

        // values outside the window are zero
        float x0 = (i - 1 < 0) ? 0.0f : input[i - 1];
        float x1 = (i - 0 < 0) ? 0.0f : input[i - 0];
        float x2 = (i + 1 < inputSize) ? input[i + 1] : 0.0f;
        float x3 = (i + 2 < inputSize) ? input[i + 2] : 0.0f;

        // compute the polynomial coefficients
        float c0 = (1/6.0f) * (x3 - x0) + (1/2.0f) * (x1 - x2);
        float c1 = (1/2.0f) * (x0 + x2) - x1;
        float c2 = x2 - (1/3.0f) * x0 - (1/2.0f) * x1 - (1/6.0f) * x3;
        float c3 = x1;

        // compute the polynomial
        float frac = f * Q32_TO_FLOAT;
        output[j] = (((c0 * frac + c1) * frac + c2) * frac + c3) * gain;

        offset += step;
    }
}

int AudioSRC::createRationalFilter(int upFactor, int downFactor, float gain) {
    int numTaps = PROTOTYPE_TAPS;
    int numPhases = upFactor;
    int numCoefs = numTaps * numPhases;
    int oldCoefs = numCoefs;
    int prototypeCoefs = PROTOTYPE_TAPS * PROTOTYPE_PHASES;

    //
    // When downsampling, we can lower the filter cutoff by downFactor/upFactor using the
    // time-scaling property of the Fourier transform.  The gain is adjusted accordingly.
    //
    if (downFactor > upFactor) {
        numCoefs = ((int64_t)oldCoefs * downFactor) / upFactor;
        numTaps = (numCoefs + upFactor - 1) / upFactor;
        gain *= (float)oldCoefs / numCoefs;
    }
    numTaps = (numTaps + 7) & ~7;   // SIMD8

    // interpolate the coefficients of the prototype filter
    float* tempFilter = new float[numTaps * numPhases];
    memset(tempFilter, 0, numTaps * numPhases * sizeof(float));

    cubicInterpolation(prototypeFilter, tempFilter, prototypeCoefs, numCoefs, gain);

    // create the polyphase filter
    _polyphaseFilter = (float*)aligned_malloc(numTaps * numPhases * sizeof(float), 32); // SIMD8

    // rearrange into polyphase form, ordered by use
    for (int i = 0; i < numPhases; i++) {
        int phase = (i * downFactor) % upFactor;
        for (int j = 0; j < numTaps; j++) {

            // the filter taps are reversed, so convolution is implemented as dot-product
            float f = tempFilter[(numTaps - j - 1) * numPhases + phase];
            _polyphaseFilter[numTaps * i + j] = f;
        }
    }

    delete[] tempFilter;

    // precompute the input steps
    _stepTable = new int[numPhases];

    for (int i = 0; i < numPhases; i++) {
        _stepTable[i] = (((int64_t)(i+1) * downFactor) / upFactor) - (((int64_t)(i+0) * downFactor) / upFactor);
    }

    return numTaps;
}

int AudioSRC::createIrrationalFilter(int upFactor, int downFactor, float gain) {
    int numTaps = PROTOTYPE_TAPS;
    int numPhases = upFactor;
    int numCoefs = numTaps * numPhases;
    int oldCoefs = numCoefs;
    int prototypeCoefs = PROTOTYPE_TAPS * PROTOTYPE_PHASES;

    //
    // When downsampling, we can lower the filter cutoff by downFactor/upFactor using the
    // time-scaling property of the Fourier transform.  The gain is adjusted accordingly.
    //
    if (downFactor > upFactor) {
        numCoefs = ((int64_t)oldCoefs * downFactor) / upFactor;
        numTaps = (numCoefs + upFactor - 1) / upFactor;
        gain *= (float)oldCoefs / numCoefs;
    }
    numTaps = (numTaps + 7) & ~7;   // SIMD8

    // interpolate the coefficients of the prototype filter
    float* tempFilter = new float[numTaps * numPhases];
    memset(tempFilter, 0, numTaps * numPhases * sizeof(float));

    cubicInterpolation(prototypeFilter, tempFilter, prototypeCoefs, numCoefs, gain);

    // create the polyphase filter, with extra phase at the end to simplify coef interpolation
    _polyphaseFilter = (float*)aligned_malloc(numTaps * (numPhases + 1) * sizeof(float), 32);   // SIMD8

    // rearrange into polyphase form, ordered by fractional delay
    for (int phase = 0; phase < numPhases; phase++) {
        for (int j = 0; j < numTaps; j++) {

            // the filter taps are reversed, so convolution is implemented as dot-product
            float f = tempFilter[(numTaps - j - 1) * numPhases + phase];
            _polyphaseFilter[numTaps * phase + j] = f;
        }
    }

    delete[] tempFilter;

    // by construction, the last tap of the first phase must be zero
    assert(_polyphaseFilter[numTaps - 1] == 0.0f);

    // so the extra phase is just the first, shifted by one
    _polyphaseFilter[numTaps * numPhases + 0] = 0.0f;
    for (int j = 1; j < numTaps; j++) {
        _polyphaseFilter[numTaps * numPhases + j] = _polyphaseFilter[j-1];
    }

    return numTaps;
}

//
// on x86 architecture, assume that SSE2 is present
//
#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)

#include <emmintrin.h>

int AudioSRC::multirateFilter1_SSE(const float* input0, float* output0, int inputFrames) {
    int outputFrames = 0;

    assert(_numTaps % 4 == 0);  // SIMD4

    if (_step == 0) {   // rational

        int32_t i = HI32(_offset);

        while (i < inputFrames) {

            const float* c0 = &_polyphaseFilter[_numTaps * _phase];

            __m128 acc0 = _mm_setzero_ps();

            for (int j = 0; j < _numTaps; j += 4) {

                //float coef = c0[j];
                __m128 coef0 = _mm_loadu_ps(&c0[j]);

                //acc += input[i + j] * coef;
                acc0 = _mm_add_ps(_mm_mul_ps(_mm_loadu_ps(&input0[i + j]), coef0), acc0);
            }

            // horizontal sum
            acc0 = _mm_add_ps(acc0, _mm_movehl_ps(acc0, acc0));
            acc0 = _mm_add_ss(acc0, _mm_shuffle_ps(acc0, acc0, _MM_SHUFFLE(0,0,0,1)));

            _mm_store_ss(&output0[outputFrames], acc0);
            outputFrames += 1;

            i += _stepTable[_phase];
            if (++_phase == _upFactor) {
                _phase = 0;
            }
        }
        _offset = (int64_t)(i - inputFrames) << 32;

    } else {    // irrational

        while (HI32(_offset) < inputFrames) {

            int32_t i = HI32(_offset);
            uint32_t f = LO32(_offset);

            uint32_t phase = f >> SRC_FRACBITS;
            __m128 frac = _mm_set1_ps((f & SRC_FRACMASK) * QFRAC_TO_FLOAT);

            const float* c0 = &_polyphaseFilter[_numTaps * (phase + 0)];
            const float* c1 = &_polyphaseFilter[_numTaps * (phase + 1)];

            __m128 acc0 = _mm_setzero_ps();

            for (int j = 0; j < _numTaps; j += 4) {

                //float coef = c0[j] + frac * (c1[j] - c0[j]);
                __m128 coef0 = _mm_loadu_ps(&c0[j]);
                __m128 coef1 = _mm_loadu_ps(&c1[j]);
                coef1 = _mm_sub_ps(coef1, coef0);
                coef0 = _mm_add_ps(_mm_mul_ps(coef1, frac), coef0);

                //acc += input[i + j] * coef;
                acc0 = _mm_add_ps(_mm_mul_ps(_mm_loadu_ps(&input0[i + j]), coef0), acc0);
            }

            // horizontal sum
            acc0 = _mm_add_ps(acc0, _mm_movehl_ps(acc0, acc0));
            acc0 = _mm_add_ss(acc0, _mm_shuffle_ps(acc0, acc0, _MM_SHUFFLE(0,0,0,1)));

            _mm_store_ss(&output0[outputFrames], acc0);
            outputFrames += 1;

            _offset += _step;
        }
        _offset -= (int64_t)inputFrames << 32;
    }

    return outputFrames;
}

int AudioSRC::multirateFilter2_SSE(const float* input0, const float* input1, float* output0, float* output1, int inputFrames) {
    int outputFrames = 0;

    assert(_numTaps % 4 == 0);  // SIMD4

    if (_step == 0) {   // rational

        int32_t i = HI32(_offset);

        while (i < inputFrames) {

            const float* c0 = &_polyphaseFilter[_numTaps * _phase];

            __m128 acc0 = _mm_setzero_ps();
            __m128 acc1 = _mm_setzero_ps();

            for (int j = 0; j < _numTaps; j += 4) {

                //float coef = c0[j];
                __m128 coef0 = _mm_loadu_ps(&c0[j]);

                //acc += input[i + j] * coef;
                acc0 = _mm_add_ps(_mm_mul_ps(_mm_loadu_ps(&input0[i + j]), coef0), acc0);
                acc1 = _mm_add_ps(_mm_mul_ps(_mm_loadu_ps(&input1[i + j]), coef0), acc1);
            }

            // horizontal sum
            acc0 = _mm_add_ps(acc0, _mm_movehl_ps(acc0, acc0));
            acc1 = _mm_add_ps(acc1, _mm_movehl_ps(acc1, acc1));
            acc0 = _mm_add_ss(acc0, _mm_shuffle_ps(acc0, acc0, _MM_SHUFFLE(0,0,0,1)));
            acc1 = _mm_add_ss(acc1, _mm_shuffle_ps(acc1, acc1, _MM_SHUFFLE(0,0,0,1)));

            _mm_store_ss(&output0[outputFrames], acc0);
            _mm_store_ss(&output1[outputFrames], acc1);
            outputFrames += 1;

            i += _stepTable[_phase];
            if (++_phase == _upFactor) {
                _phase = 0;
            }
        }
        _offset = (int64_t)(i - inputFrames) << 32;

    } else {    // irrational

        while (HI32(_offset) < inputFrames) {

            int32_t i = HI32(_offset);
            uint32_t f = LO32(_offset);

            uint32_t phase = f >> SRC_FRACBITS;
            __m128 frac = _mm_set1_ps((f & SRC_FRACMASK) * QFRAC_TO_FLOAT);

            const float* c0 = &_polyphaseFilter[_numTaps * (phase + 0)];
            const float* c1 = &_polyphaseFilter[_numTaps * (phase + 1)];

            __m128 acc0 = _mm_setzero_ps();
            __m128 acc1 = _mm_setzero_ps();

            for (int j = 0; j < _numTaps; j += 4) {

                //float coef = c0[j] + frac * (c1[j] - c0[j]);
                __m128 coef0 = _mm_loadu_ps(&c0[j]);
                __m128 coef1 = _mm_loadu_ps(&c1[j]);
                coef1 = _mm_sub_ps(coef1, coef0);
                coef0 = _mm_add_ps(_mm_mul_ps(coef1, frac), coef0);

                //acc += input[i + j] * coef;
                acc0 = _mm_add_ps(_mm_mul_ps(_mm_loadu_ps(&input0[i + j]), coef0), acc0);
                acc1 = _mm_add_ps(_mm_mul_ps(_mm_loadu_ps(&input1[i + j]), coef0), acc1);
            }

            // horizontal sum
            acc0 = _mm_add_ps(acc0, _mm_movehl_ps(acc0, acc0));
            acc1 = _mm_add_ps(acc1, _mm_movehl_ps(acc1, acc1));
            acc0 = _mm_add_ss(acc0, _mm_shuffle_ps(acc0, acc0, _MM_SHUFFLE(0,0,0,1)));
            acc1 = _mm_add_ss(acc1, _mm_shuffle_ps(acc1, acc1, _MM_SHUFFLE(0,0,0,1)));

            _mm_store_ss(&output0[outputFrames], acc0);
            _mm_store_ss(&output1[outputFrames], acc1);
            outputFrames += 1;

            _offset += _step;
        }
        _offset -= (int64_t)inputFrames << 32;
    }

    return outputFrames;
}

//
// Runtime CPU dispatch
//

#include "CPUDetect.h"

int AudioSRC::multirateFilter1(const float* input0, float* output0, int inputFrames) {

    static auto f = cpuSupportsAVX2() ? &AudioSRC::multirateFilter1_AVX2 : &AudioSRC::multirateFilter1_SSE;
    return (this->*f)(input0, output0, inputFrames);    // dispatch
}

int AudioSRC::multirateFilter2(const float* input0, const float* input1, float* output0, float* output1, int inputFrames) {

    static auto f = cpuSupportsAVX2() ? &AudioSRC::multirateFilter2_AVX2 : &AudioSRC::multirateFilter2_SSE;
    return (this->*f)(input0, input1, output0, output1, inputFrames);   // dispatch
}

// convert int16_t to float, deinterleave stereo
void AudioSRC::convertInput(const int16_t* input, float** outputs, int numFrames) {
    __m128 scale = _mm_set1_ps(1/32768.0f);

    if (_numChannels == 1) {

        int i = 0;
        for (; i < numFrames - 3; i += 4) {
            __m128i a0 = _mm_loadl_epi64((__m128i*)&input[i]);

            // sign-extend
            a0 = _mm_srai_epi32(_mm_unpacklo_epi16(a0, a0), 16);

            __m128 f0 = _mm_mul_ps(_mm_cvtepi32_ps(a0), scale);

            _mm_storeu_ps(&outputs[0][i], f0);
        }
        for (; i < numFrames; i++) {
            __m128i a0 = _mm_insert_epi16(_mm_setzero_si128(), input[i], 0);

            // sign-extend
            a0 = _mm_srai_epi32(_mm_unpacklo_epi16(a0, a0), 16);

            __m128 f0 = _mm_mul_ps(_mm_cvtepi32_ps(a0), scale);

            _mm_store_ss(&outputs[0][i], f0);
        }

    } else if (_numChannels == 2) {

        int i = 0;
        for (; i < numFrames - 3; i += 4) {
            __m128i a0 = _mm_loadu_si128((__m128i*)&input[2*i]);
            __m128i a1 = a0;

            // deinterleave and sign-extend
            a0 = _mm_madd_epi16(a0, _mm_set1_epi32(0x00000001));
            a1 = _mm_madd_epi16(a1, _mm_set1_epi32(0x00010000));

            __m128 f0 = _mm_mul_ps(_mm_cvtepi32_ps(a0), scale);
            __m128 f1 = _mm_mul_ps(_mm_cvtepi32_ps(a1), scale);

            _mm_storeu_ps(&outputs[0][i], f0);
            _mm_storeu_ps(&outputs[1][i], f1);
        }
        for (; i < numFrames; i++) {
            __m128i a0 = _mm_cvtsi32_si128(*(int32_t*)&input[2*i]);
            __m128i a1 = a0;

            // deinterleave and sign-extend
            a0 = _mm_madd_epi16(a0, _mm_set1_epi32(0x00000001));
            a1 = _mm_madd_epi16(a1, _mm_set1_epi32(0x00010000));

            __m128 f0 = _mm_mul_ps(_mm_cvtepi32_ps(a0), scale);
            __m128 f1 = _mm_mul_ps(_mm_cvtepi32_ps(a1), scale);

            _mm_store_ss(&outputs[0][i], f0);
            _mm_store_ss(&outputs[1][i], f1);
        }
    }
}

// fast TPDF dither in [-1.0f, 1.0f]
static inline __m128 dither4() {
    static __m128i rz;

    // update the 8 different maximum-length LCGs
    rz = _mm_mullo_epi16(rz, _mm_set_epi16(25173, -25511, -5975, -23279, 19445, -27591, 30185, -3495));
    rz = _mm_add_epi16(rz, _mm_set_epi16(13849, -32767, 105, -19675, -7701, -32679, -13225, 28013));

    // promote to 32-bit
    __m128i r0 = _mm_unpacklo_epi16(rz, _mm_setzero_si128());
    __m128i r1 = _mm_unpackhi_epi16(rz, _mm_setzero_si128());

    // return (r0 - r1) * (1/65536.0f);
    __m128 d0 = _mm_cvtepi32_ps(_mm_sub_epi32(r0, r1));
    return _mm_mul_ps(d0, _mm_set1_ps(1/65536.0f));
}

// convert float to int16_t with dither, interleave stereo
void AudioSRC::convertOutput(float** inputs, int16_t* output, int numFrames) {
    __m128 scale = _mm_set1_ps(32768.0f);

    if (_numChannels == 1) {

        int i = 0;
        for (; i < numFrames - 3; i += 4) {
            __m128 f0 = _mm_mul_ps(_mm_loadu_ps(&inputs[0][i]), scale);

            f0 = _mm_add_ps(f0, dither4());

            // round and saturate
            __m128i a0 = _mm_cvtps_epi32(f0);
            a0 = _mm_packs_epi32(a0, a0);

            _mm_storel_epi64((__m128i*)&output[i], a0);
        }
        for (; i < numFrames; i++) {
            __m128 f0 = _mm_mul_ps(_mm_load_ss(&inputs[0][i]), scale);

            f0 = _mm_add_ps(f0, dither4());

            // round and saturate
            __m128i a0 = _mm_cvtps_epi32(f0);
            a0 = _mm_packs_epi32(a0, a0);

            output[i] = (int16_t)_mm_extract_epi16(a0, 0);
        }

    } else if (_numChannels == 2) {

        int i = 0;
        for (; i < numFrames - 3; i += 4) {
            __m128 f0 = _mm_mul_ps(_mm_loadu_ps(&inputs[0][i]), scale);
            __m128 f1 = _mm_mul_ps(_mm_loadu_ps(&inputs[1][i]), scale);

            __m128 d0 = dither4();
            f0 = _mm_add_ps(f0, d0);
            f1 = _mm_add_ps(f1, d0);

            // round and saturate
            __m128i a0 = _mm_cvtps_epi32(f0);
            __m128i a1 = _mm_cvtps_epi32(f1);
            a0 = _mm_packs_epi32(a0, a0);
            a1 = _mm_packs_epi32(a1, a1);

            // interleave
            a0 = _mm_unpacklo_epi16(a0, a1);
            _mm_storeu_si128((__m128i*)&output[2*i], a0);
        }
        for (; i < numFrames; i++) {
            __m128 f0 = _mm_mul_ps(_mm_load_ss(&inputs[0][i]), scale);
            __m128 f1 = _mm_mul_ps(_mm_load_ss(&inputs[1][i]), scale);

            __m128 d0 = dither4();
            f0 = _mm_add_ps(f0, d0);
            f1 = _mm_add_ps(f1, d0);

            // round and saturate
            __m128i a0 = _mm_cvtps_epi32(f0);
            __m128i a1 = _mm_cvtps_epi32(f1);
            a0 = _mm_packs_epi32(a0, a0);
            a1 = _mm_packs_epi32(a1, a1);

            // interleave
            a0 = _mm_unpacklo_epi16(a0, a1);
            *(int32_t*)&output[2*i] = _mm_cvtsi128_si32(a0);
        }
    }
}

// deinterleave stereo
void AudioSRC::convertInput(const float* input, float** outputs, int numFrames) {

    if (_numChannels == 1) {

        memcpy(outputs[0], input, numFrames * sizeof(float));

    } else if (_numChannels == 2) {

        int i = 0;
        for (; i < numFrames - 3; i += 4) {
            __m128 f0 = _mm_loadu_ps(&input[2*i + 0]);
            __m128 f1 = _mm_loadu_ps(&input[2*i + 4]);

            // deinterleave
            _mm_storeu_ps(&outputs[0][i], _mm_shuffle_ps(f0, f1, _MM_SHUFFLE(2,0,2,0)));
            _mm_storeu_ps(&outputs[1][i], _mm_shuffle_ps(f0, f1, _MM_SHUFFLE(3,1,3,1)));
        }
        for (; i < numFrames; i++) {
            // deinterleave
            outputs[0][i] = input[2*i + 0];
            outputs[1][i] = input[2*i + 1];
        }
    }
}

// interleave stereo
void AudioSRC::convertOutput(float** inputs, float* output, int numFrames) {

    if (_numChannels == 1) {

        memcpy(output, inputs[0], numFrames * sizeof(float));

    } else if (_numChannels == 2) {

        int i = 0;
        for (; i < numFrames - 3; i += 4) {
            __m128 f0 = _mm_loadu_ps(&inputs[0][i]);
            __m128 f1 = _mm_loadu_ps(&inputs[1][i]);

            // interleave
            _mm_storeu_ps(&output[2*i + 0], _mm_unpacklo_ps(f0, f1));
            _mm_storeu_ps(&output[2*i + 4], _mm_unpackhi_ps(f0, f1));
        }
        for (; i < numFrames; i++) {
            // interleave
            output[2*i + 0] = inputs[0][i];
            output[2*i + 1] = inputs[1][i];
        }
    }
}

#else

int AudioSRC::multirateFilter1(const float* input0, float* output0, int inputFrames) {
    int outputFrames = 0;

    if (_step == 0) {   // rational

        int32_t i = HI32(_offset);

        while (i < inputFrames) {

            const float* c0 = &_polyphaseFilter[_numTaps * _phase];

            float acc0 = 0.0f;

            for (int j = 0; j < _numTaps; j++) {

                float coef = c0[j];

                acc0 += input0[i + j] * coef;
            }

            output0[outputFrames] = acc0;
            outputFrames += 1;

            i += _stepTable[_phase];
            if (++_phase == _upFactor) {
                _phase = 0;
            }
        }
        _offset = (int64_t)(i - inputFrames) << 32;

    } else {    // irrational

        while (HI32(_offset) < inputFrames) {

            int32_t i = HI32(_offset);
            uint32_t f = LO32(_offset);

            uint32_t phase = f >> SRC_FRACBITS;
            float frac = (f & SRC_FRACMASK) * QFRAC_TO_FLOAT;

            const float* c0 = &_polyphaseFilter[_numTaps * (phase + 0)];
            const float* c1 = &_polyphaseFilter[_numTaps * (phase + 1)];

            float acc0 = 0.0f;

            for (int j = 0; j < _numTaps; j++) {

                float coef = c0[j] + frac * (c1[j] - c0[j]);

                acc0 += input0[i + j] * coef;
            }

            output0[outputFrames] = acc0;
            outputFrames += 1;

            _offset += _step;
        }
        _offset -= (int64_t)inputFrames << 32;
    }

    return outputFrames;
}

int AudioSRC::multirateFilter2(const float* input0, const float* input1, float* output0, float* output1, int inputFrames) {
    int outputFrames = 0;

    if (_step == 0) {   // rational

        int32_t i = HI32(_offset);

        while (i < inputFrames) {

            const float* c0 = &_polyphaseFilter[_numTaps * _phase];

            float acc0 = 0.0f;
            float acc1 = 0.0f;

            for (int j = 0; j < _numTaps; j++) {

                float coef = c0[j];

                acc0 += input0[i + j] * coef;
                acc1 += input1[i + j] * coef;
            }

            output0[outputFrames] = acc0;
            output1[outputFrames] = acc1;
            outputFrames += 1;

            i += _stepTable[_phase];
            if (++_phase == _upFactor) {
                _phase = 0;
            }
        }
        _offset = (int64_t)(i - inputFrames) << 32;

    } else {    // irrational

        while (HI32(_offset) < inputFrames) {

            int32_t i = HI32(_offset);
            uint32_t f = LO32(_offset);

            uint32_t phase = f >> SRC_FRACBITS;
            float frac = (f & SRC_FRACMASK) * QFRAC_TO_FLOAT;

            const float* c0 = &_polyphaseFilter[_numTaps * (phase + 0)];
            const float* c1 = &_polyphaseFilter[_numTaps * (phase + 1)];

            float acc0 = 0.0f;
            float acc1 = 0.0f;

            for (int j = 0; j < _numTaps; j++) {

                float coef = c0[j] + frac * (c1[j] - c0[j]);

                acc0 += input0[i + j] * coef;
                acc1 += input1[i + j] * coef;
            }

            output0[outputFrames] = acc0;
            output1[outputFrames] = acc1;
            outputFrames += 1;

            _offset += _step;
        }
        _offset -= (int64_t)inputFrames << 32;
    }

    return outputFrames;
}

// convert int16_t to float, deinterleave stereo
void AudioSRC::convertInput(const int16_t* input, float** outputs, int numFrames) {
    const float scale = 1/32768.0f;

    if (_numChannels == 1) {
        for (int i = 0; i < numFrames; i++) {
            outputs[0][i] = (float)input[i] * scale;
        }
    } else if (_numChannels == 2) {
        for (int i = 0; i < numFrames; i++) {
            outputs[0][i] = (float)input[2*i + 0] * scale;
            outputs[1][i] = (float)input[2*i + 1] * scale;
        }
    }
}

// fast TPDF dither in [-1.0f, 1.0f]
static inline float dither() {
    static uint32_t rz = 0;
    rz = rz * 69069 + 1;
    int32_t r0 = rz & 0xffff;
    int32_t r1 = rz >> 16;
    return (int32_t)(r0 - r1) * (1/65536.0f);
}

// convert float to int16_t with dither, interleave stereo
void AudioSRC::convertOutput(float** inputs, int16_t* output, int numFrames) {
    const float scale = 32768.0f;

    if (_numChannels == 1) {
        for (int i = 0; i < numFrames; i++) {

            float f = inputs[0][i] * scale;

            f += dither();

            // round and saturate
            f += (f < 0.0f ? -0.5f : +0.5f);
            f = std::max(std::min(f, 32767.0f), -32768.0f);

            output[i] = (int16_t)f;
        }
    } else if (_numChannels == 2) {
        for (int i = 0; i < numFrames; i++) {

            float f0 = inputs[0][i] * scale;
            float f1 = inputs[1][i] * scale;

            float d = dither();
            f0 += d;
            f1 += d;

            // round and saturate
            f0 += (f0 < 0.0f ? -0.5f : +0.5f);
            f1 += (f1 < 0.0f ? -0.5f : +0.5f);
            f0 = std::max(std::min(f0, 32767.0f), -32768.0f);
            f1 = std::max(std::min(f1, 32767.0f), -32768.0f);

            // interleave
            output[2*i + 0] = (int16_t)f0;
            output[2*i + 1] = (int16_t)f1;
        }
    }
}

// deinterleave stereo
void AudioSRC::convertInput(const float* input, float** outputs, int numFrames) {

    if (_numChannels == 1) {

        memcpy(outputs[0], input, numFrames * sizeof(float));

    } else if (_numChannels == 2) {
        for (int i = 0; i < numFrames; i++) {
            // deinterleave
            outputs[0][i] = input[2*i + 0];
            outputs[1][i] = input[2*i + 1];
        }
    }
}

// interleave stereo
void AudioSRC::convertOutput(float** inputs, float* output, int numFrames) {

    if (_numChannels == 1) {

        memcpy(output, inputs[0], numFrames * sizeof(float));

    } else if (_numChannels == 2) {
        for (int i = 0; i < numFrames; i++) {
            // interleave
            output[2*i + 0] = inputs[0][i];
            output[2*i + 1] = inputs[1][i];
        }
    }
}

#endif

int AudioSRC::render(float** inputs, float** outputs, int inputFrames) {
    int outputFrames = 0;

    int nh = std::min(_numHistory, inputFrames);    // number of frames from history buffer
    int ni = inputFrames - nh;                      // number of frames from remaining input

    if (_numChannels == 1) {

        // refill history buffers
        memcpy(_history[0] + _numHistory, inputs[0], nh * sizeof(float));

        // process history buffer
        outputFrames += multirateFilter1(_history[0], outputs[0], nh);

        // process remaining input
        if (ni) {
            outputFrames += multirateFilter1(inputs[0], outputs[0] + outputFrames, ni);
        }

        // shift history buffers
        if (ni) {
            memcpy(_history[0], inputs[0] + ni, _numHistory * sizeof(float));
        } else {
            memmove(_history[0], _history[0] + nh, _numHistory * sizeof(float));
        }

    } else if (_numChannels == 2) {

        // refill history buffers
        memcpy(_history[0] + _numHistory, inputs[0], nh * sizeof(float));
        memcpy(_history[1] + _numHistory, inputs[1], nh * sizeof(float));

        // process history buffer
        outputFrames += multirateFilter2(_history[0], _history[1], outputs[0], outputs[1], nh);

        // process remaining input
        if (ni) {
            outputFrames += multirateFilter2(inputs[0], inputs[1], outputs[0] + outputFrames, outputs[1] + outputFrames, ni);
        }

        // shift history buffers
        if (ni) {
            memcpy(_history[0], inputs[0] + ni, _numHistory * sizeof(float));
            memcpy(_history[1], inputs[1] + ni, _numHistory * sizeof(float));
        } else {
            memmove(_history[0], _history[0] + nh, _numHistory * sizeof(float));
            memmove(_history[1], _history[1] + nh, _numHistory * sizeof(float));
        }
    }

    return outputFrames;
}

AudioSRC::AudioSRC(int inputSampleRate, int outputSampleRate, int numChannels) {
    assert(inputSampleRate > 0);
    assert(outputSampleRate > 0);
    assert(numChannels > 0);
    assert(numChannels <= SRC_MAX_CHANNELS);

    _inputSampleRate = inputSampleRate;
    _outputSampleRate = outputSampleRate;
    _numChannels = numChannels;

    // reduce to the smallest rational fraction
    int divisor = gcd(inputSampleRate, outputSampleRate);
    _upFactor = outputSampleRate / divisor;
    _downFactor = inputSampleRate / divisor;
    _step = 0;  // rational mode

    // if the number of phases is too large, use irrational mode
    if (_upFactor > 640) {
        _upFactor = SRC_PHASES;
        _downFactor = ((int64_t)SRC_PHASES * _inputSampleRate) / _outputSampleRate;
        _step = ((int64_t)_inputSampleRate << 32) / _outputSampleRate;
    }

    _polyphaseFilter = nullptr;
    _stepTable = nullptr;

    // create the polyphase filter
    if (_step == 0) {
        _numTaps = createRationalFilter(_upFactor, _downFactor, 1.0f);
    } else {
        _numTaps = createIrrationalFilter(_upFactor, _downFactor, 1.0f);
    }

    //printf("up=%d down=%.3f taps=%d\n", _upFactor, _downFactor + (LO32(_step)<<SRC_PHASEBITS) * Q32_TO_FLOAT, _numTaps);

    // filter history buffers
    _numHistory = _numTaps - 1;
    _history[0] = new float[2 * _numHistory];
    _history[1] = new float[2 * _numHistory];

    // format conversion buffers
    _inputs[0] = (float*)aligned_malloc(4*SRC_BLOCK * sizeof(float), 16); // SIMD4
    _inputs[1] = _inputs[0] + 1*SRC_BLOCK;
    _outputs[0] = _inputs[0] + 2*SRC_BLOCK;
    _outputs[1] = _inputs[0] + 3*SRC_BLOCK;

    // input blocking size, such that input and output are both guaranteed not to exceed SRC_BLOCK frames
    _inputBlock = std::min(SRC_BLOCK, getMaxInput(SRC_BLOCK));

    // reset the state
    _offset = 0;
    _phase = 0;
    memset(_history[0], 0, 2 * _numHistory * sizeof(float));
    memset(_history[1], 0, 2 * _numHistory * sizeof(float));
}

AudioSRC::~AudioSRC() {
    aligned_free(_polyphaseFilter);
    delete[] _stepTable;

    delete[] _history[0];
    delete[] _history[1];

    aligned_free(_inputs[0]);
}

//
// This version handles input/output as interleaved int16_t
//
int AudioSRC::render(const int16_t* input, int16_t* output, int inputFrames) {
    int outputFrames = 0;

    while (inputFrames) {

        int ni = std::min(inputFrames, _inputBlock);

        convertInput(input, _inputs, ni);

        int no = render(_inputs, _outputs, ni);
        assert(no <= SRC_BLOCK);

        convertOutput(_outputs, output, no);

        input += _numChannels * ni;
        output += _numChannels * no;
        inputFrames -= ni;
        outputFrames += no;
    }

    return outputFrames;
}

//
// This version handles input/output as interleaved float
//
int AudioSRC::render(const float* input, float* output, int inputFrames) {
    int outputFrames = 0;

    while (inputFrames) {

        int ni = std::min(inputFrames, _inputBlock);

        convertInput(input, _inputs, ni);

        int no = render(_inputs, _outputs, ni);
        assert(no <= SRC_BLOCK);

        convertOutput(_outputs, output, no);

        input += _numChannels * ni;
        output += _numChannels * no;
        inputFrames -= ni;
        outputFrames += no;
    }

    return outputFrames;
}

// the min output frames that will be produced by inputFrames
int AudioSRC::getMinOutput(int inputFrames) {
    if (_step == 0) {
        return (int)(((int64_t)inputFrames * _upFactor) / _downFactor);
    } else {
        return (int)(((int64_t)inputFrames << 32) / _step);
    }
}

// the max output frames that will be produced by inputFrames
int AudioSRC::getMaxOutput(int inputFrames) {
    if (_step == 0) {
        return (int)(((int64_t)inputFrames * _upFactor + _downFactor-1) / _downFactor);
    } else {
        return (int)((((int64_t)inputFrames << 32) + _step-1) / _step);
    }
}

// the min input frames that will produce at least outputFrames
int AudioSRC::getMinInput(int outputFrames) {
    if (_step == 0) {
        return (int)(((int64_t)outputFrames * _downFactor + _upFactor-1) / _upFactor);
    } else {
        return (int)(((int64_t)outputFrames * _step + 0xffffffffu) >> 32);
    }
}

// the max input frames that will produce at most outputFrames
int AudioSRC::getMaxInput(int outputFrames) {
    if (_step == 0) {
        return (int)(((int64_t)outputFrames * _downFactor) / _upFactor);
    } else {
        return (int)(((int64_t)outputFrames * _step) >> 32);
    }
}
