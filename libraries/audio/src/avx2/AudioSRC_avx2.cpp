//
//  AudioSRC_avx2.cpp
//  libraries/audio/src
//
//  Created by Ken Cooke on 6/5/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)

#include <assert.h>
#include <immintrin.h>

#include "../AudioSRC.h"

#ifndef __AVX2__
#error Must be compiled with /arch:AVX2 or -mavx2 -mfma.
#endif

// high/low part of int64_t
#define LO32(a)   ((uint32_t)(a))
#define HI32(a)   ((int32_t)((a) >> 32))

int AudioSRC::multirateFilter1_AVX2(const float* input0, float* output0, int inputFrames) {
    int outputFrames = 0;

    assert(_numTaps % 8 == 0);  // SIMD8

    if (_step == 0) {   // rational

        int32_t i = HI32(_offset);

        while (i < inputFrames) {

            const float* c0 = &_polyphaseFilter[_numTaps * _phase];

            __m256 acc0 = _mm256_setzero_ps();

            for (int j = 0; j < _numTaps; j += 8) {

                //float coef = c0[j];
                __m256 coef0 = _mm256_loadu_ps(&c0[j]);

                //acc += input[i + j] * coef;
                acc0 = _mm256_fmadd_ps(_mm256_loadu_ps(&input0[i + j]), coef0, acc0);
            }

            // horizontal sum
            acc0 = _mm256_hadd_ps(acc0, acc0);
            __m128 t0 = _mm_add_ps(_mm256_castps256_ps128(acc0), _mm256_extractf128_ps(acc0, 1));
            t0 = _mm_add_ps(t0, _mm_movehdup_ps(t0));

            _mm_store_ss(&output0[outputFrames], t0);
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
            float ftmp = (f & SRC_FRACMASK) * QFRAC_TO_FLOAT;

            const float* c0 = &_polyphaseFilter[_numTaps * (phase + 0)];
            const float* c1 = &_polyphaseFilter[_numTaps * (phase + 1)];

            __m256 acc0 = _mm256_setzero_ps();
            __m256 frac = _mm256_broadcast_ss(&ftmp);

            for (int j = 0; j < _numTaps; j += 8) {

                //float coef = c0[j] + frac * (c1[j] - c0[j]);
                __m256 coef0 = _mm256_loadu_ps(&c0[j]);
                __m256 coef1 = _mm256_loadu_ps(&c1[j]);
                coef1 = _mm256_sub_ps(coef1, coef0);
                coef0 = _mm256_fmadd_ps(coef1, frac, coef0);

                //acc += input[i + j] * coef;
                acc0 = _mm256_fmadd_ps(_mm256_loadu_ps(&input0[i + j]), coef0, acc0);
            }

            // horizontal sum
            acc0 = _mm256_hadd_ps(acc0, acc0);
            __m128 t0 = _mm_add_ps(_mm256_castps256_ps128(acc0), _mm256_extractf128_ps(acc0, 1));
            t0 = _mm_add_ps(t0, _mm_movehdup_ps(t0));

            _mm_store_ss(&output0[outputFrames], t0);
            outputFrames += 1;

            _offset += _step;
        }
        _offset -= (int64_t)inputFrames << 32;
    }
    _mm256_zeroupper();

    return outputFrames;
}

int AudioSRC::multirateFilter2_AVX2(const float* input0, const float* input1, float* output0, float* output1, int inputFrames) {
    int outputFrames = 0;

    assert(_numTaps % 8 == 0);  // SIMD8

    if (_step == 0) {   // rational

        int32_t i = HI32(_offset);

        while (i < inputFrames) {

            const float* c0 = &_polyphaseFilter[_numTaps * _phase];

            __m256 acc0 = _mm256_setzero_ps();
            __m256 acc1 = _mm256_setzero_ps();

            for (int j = 0; j < _numTaps; j += 8) {

                //float coef = c0[j];
                __m256 coef0 = _mm256_loadu_ps(&c0[j]);

                //acc += input[i + j] * coef;
                acc0 = _mm256_fmadd_ps(_mm256_loadu_ps(&input0[i + j]), coef0, acc0);
                acc1 = _mm256_fmadd_ps(_mm256_loadu_ps(&input1[i + j]), coef0, acc1);
            }

            // horizontal sum
            acc0 = _mm256_hadd_ps(acc0, acc1);
            __m128 t0 = _mm_add_ps(_mm256_castps256_ps128(acc0), _mm256_extractf128_ps(acc0, 1));
            t0 = _mm_add_ps(t0, _mm_movehdup_ps(t0));

            _mm_store_ss(&output0[outputFrames], t0);
            _mm_store_ss(&output1[outputFrames], _mm_shuffle_ps(t0, t0, _MM_SHUFFLE(0,0,0,2)));
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
            float ftmp = (f & SRC_FRACMASK) * QFRAC_TO_FLOAT;

            const float* c0 = &_polyphaseFilter[_numTaps * (phase + 0)];
            const float* c1 = &_polyphaseFilter[_numTaps * (phase + 1)];

            __m256 acc0 = _mm256_setzero_ps();
            __m256 acc1 = _mm256_setzero_ps();
            __m256 frac = _mm256_broadcast_ss(&ftmp);

            for (int j = 0; j < _numTaps; j += 8) {

                //float coef = c0[j] + frac * (c1[j] - c0[j]);
                __m256 coef0 = _mm256_loadu_ps(&c0[j]);
                __m256 coef1 = _mm256_loadu_ps(&c1[j]);
                coef1 = _mm256_sub_ps(coef1, coef0);
                coef0 = _mm256_fmadd_ps(coef1, frac, coef0);

                //acc += input[i + j] * coef;
                acc0 = _mm256_fmadd_ps(_mm256_loadu_ps(&input0[i + j]), coef0, acc0);
                acc1 = _mm256_fmadd_ps(_mm256_loadu_ps(&input1[i + j]), coef0, acc1);
            }

            // horizontal sum
            acc0 = _mm256_hadd_ps(acc0, acc1);
            __m128 t0 = _mm_add_ps(_mm256_castps256_ps128(acc0), _mm256_extractf128_ps(acc0, 1));
            t0 = _mm_add_ps(t0, _mm_movehdup_ps(t0));

            _mm_store_ss(&output0[outputFrames], t0);
            _mm_store_ss(&output1[outputFrames], _mm_shuffle_ps(t0, t0, _MM_SHUFFLE(0,0,0,2)));
            outputFrames += 1;

            _offset += _step;
        }
        _offset -= (int64_t)inputFrames << 32;
    }
    _mm256_zeroupper();

    return outputFrames;
}

int AudioSRC::multirateFilter4_AVX2(const float* input0, const float* input1, const float* input2, const float* input3, 
                                    float* output0, float* output1, float* output2, float* output3, int inputFrames) {
    int outputFrames = 0;

    assert(_numTaps % 8 == 0);  // SIMD8

    if (_step == 0) {   // rational

        int32_t i = HI32(_offset);

        while (i < inputFrames) {

            const float* c0 = &_polyphaseFilter[_numTaps * _phase];

            __m256 acc0 = _mm256_setzero_ps();
            __m256 acc1 = _mm256_setzero_ps();
            __m256 acc2 = _mm256_setzero_ps();
            __m256 acc3 = _mm256_setzero_ps();

            for (int j = 0; j < _numTaps; j += 8) {

                //float coef = c0[j];
                __m256 coef0 = _mm256_loadu_ps(&c0[j]);

                //acc += input[i + j] * coef;
                acc0 = _mm256_fmadd_ps(_mm256_loadu_ps(&input0[i + j]), coef0, acc0);
                acc1 = _mm256_fmadd_ps(_mm256_loadu_ps(&input1[i + j]), coef0, acc1);
                acc2 = _mm256_fmadd_ps(_mm256_loadu_ps(&input2[i + j]), coef0, acc2);
                acc3 = _mm256_fmadd_ps(_mm256_loadu_ps(&input3[i + j]), coef0, acc3);
            }

            // horizontal sum
            acc0 = _mm256_hadd_ps(acc0, acc1);
            acc2 = _mm256_hadd_ps(acc2, acc3);
            acc0 = _mm256_hadd_ps(acc0, acc2);
            __m128 t0 = _mm_add_ps(_mm256_castps256_ps128(acc0), _mm256_extractf128_ps(acc0, 1));

            _mm_store_ss(&output0[outputFrames], t0);
            _mm_store_ss(&output1[outputFrames], _mm_shuffle_ps(t0, t0, _MM_SHUFFLE(0,0,0,1)));
            _mm_store_ss(&output2[outputFrames], _mm_shuffle_ps(t0, t0, _MM_SHUFFLE(0,0,0,2)));
            _mm_store_ss(&output3[outputFrames], _mm_shuffle_ps(t0, t0, _MM_SHUFFLE(0,0,0,3)));
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
            float ftmp = (f & SRC_FRACMASK) * QFRAC_TO_FLOAT;

            const float* c0 = &_polyphaseFilter[_numTaps * (phase + 0)];
            const float* c1 = &_polyphaseFilter[_numTaps * (phase + 1)];

            __m256 acc0 = _mm256_setzero_ps();
            __m256 acc1 = _mm256_setzero_ps();
            __m256 acc2 = _mm256_setzero_ps();
            __m256 acc3 = _mm256_setzero_ps();
            __m256 frac = _mm256_broadcast_ss(&ftmp);

            for (int j = 0; j < _numTaps; j += 8) {

                //float coef = c0[j] + frac * (c1[j] - c0[j]);
                __m256 coef0 = _mm256_loadu_ps(&c0[j]);
                __m256 coef1 = _mm256_loadu_ps(&c1[j]);
                coef1 = _mm256_sub_ps(coef1, coef0);
                coef0 = _mm256_fmadd_ps(coef1, frac, coef0);

                //acc += input[i + j] * coef;
                acc0 = _mm256_fmadd_ps(_mm256_loadu_ps(&input0[i + j]), coef0, acc0);
                acc1 = _mm256_fmadd_ps(_mm256_loadu_ps(&input1[i + j]), coef0, acc1);
                acc2 = _mm256_fmadd_ps(_mm256_loadu_ps(&input2[i + j]), coef0, acc2);
                acc3 = _mm256_fmadd_ps(_mm256_loadu_ps(&input3[i + j]), coef0, acc3);
            }

            // horizontal sum
            acc0 = _mm256_hadd_ps(acc0, acc1);
            acc2 = _mm256_hadd_ps(acc2, acc3);
            acc0 = _mm256_hadd_ps(acc0, acc2);
            __m128 t0 = _mm_add_ps(_mm256_castps256_ps128(acc0), _mm256_extractf128_ps(acc0, 1));

            _mm_store_ss(&output0[outputFrames], t0);
            _mm_store_ss(&output1[outputFrames], _mm_shuffle_ps(t0, t0, _MM_SHUFFLE(0,0,0,1)));
            _mm_store_ss(&output2[outputFrames], _mm_shuffle_ps(t0, t0, _MM_SHUFFLE(0,0,0,2)));
            _mm_store_ss(&output3[outputFrames], _mm_shuffle_ps(t0, t0, _MM_SHUFFLE(0,0,0,3)));
            outputFrames += 1;

            _offset += _step;
        }
        _offset -= (int64_t)inputFrames << 32;
    }
    _mm256_zeroupper();

    return outputFrames;
}

#endif
