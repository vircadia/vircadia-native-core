//
//  AudioHRTF_avx512.cpp
//  libraries/audio/src
//
//  Created by Ken Cooke on 6/20/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#if defined(__AVX512F__)

#include <assert.h>
#include <immintrin.h>

#include "../AudioHRTF.h"

#if defined(__GNUC__) && !defined(__clang__)
// for some reason, GCC -O2 results in poorly optimized code
#pragma GCC optimize("Os")
#endif

// 1 channel input, 4 channel output
void FIR_1x4_AVX512(float* src, float* dst0, float* dst1, float* dst2, float* dst3, float coef[4][HRTF_TAPS], int numFrames) {

    float* coef0 = coef[0] + HRTF_TAPS - 1;     // process backwards
    float* coef1 = coef[1] + HRTF_TAPS - 1;
    float* coef2 = coef[2] + HRTF_TAPS - 1;
    float* coef3 = coef[3] + HRTF_TAPS - 1;

    assert(numFrames % 16 == 0);

    for (int i = 0; i < numFrames; i += 16) {

        __m512 acc0 = _mm512_setzero_ps();
        __m512 acc1 = _mm512_setzero_ps();
        __m512 acc2 = _mm512_setzero_ps();
        __m512 acc3 = _mm512_setzero_ps();
        __m512 acc4 = _mm512_setzero_ps();
        __m512 acc5 = _mm512_setzero_ps();
        __m512 acc6 = _mm512_setzero_ps();
        __m512 acc7 = _mm512_setzero_ps();

        float* ps = &src[i - HRTF_TAPS + 1];    // process forwards

        assert(HRTF_TAPS % 4 == 0);

        for (int k = 0; k < HRTF_TAPS; k += 4) {

            __m512 x0 = _mm512_loadu_ps(&ps[k+0]);
            acc0 = _mm512_fmadd_ps(_mm512_set1_ps(coef0[-k-0]), x0, acc0);  // vfmadd231ps acc0, x0, dword ptr [coef]{1to16}
            acc1 = _mm512_fmadd_ps(_mm512_set1_ps(coef1[-k-0]), x0, acc1);
            acc2 = _mm512_fmadd_ps(_mm512_set1_ps(coef2[-k-0]), x0, acc2);
            acc3 = _mm512_fmadd_ps(_mm512_set1_ps(coef3[-k-0]), x0, acc3);

            __m512 x1 = _mm512_loadu_ps(&ps[k+1]);
            acc4 = _mm512_fmadd_ps(_mm512_set1_ps(coef0[-k-1]), x1, acc4);
            acc5 = _mm512_fmadd_ps(_mm512_set1_ps(coef1[-k-1]), x1, acc5);
            acc6 = _mm512_fmadd_ps(_mm512_set1_ps(coef2[-k-1]), x1, acc6);
            acc7 = _mm512_fmadd_ps(_mm512_set1_ps(coef3[-k-1]), x1, acc7);

            __m512 x2 = _mm512_loadu_ps(&ps[k+2]);
            acc0 = _mm512_fmadd_ps(_mm512_set1_ps(coef0[-k-2]), x2, acc0);
            acc1 = _mm512_fmadd_ps(_mm512_set1_ps(coef1[-k-2]), x2, acc1);
            acc2 = _mm512_fmadd_ps(_mm512_set1_ps(coef2[-k-2]), x2, acc2);
            acc3 = _mm512_fmadd_ps(_mm512_set1_ps(coef3[-k-2]), x2, acc3);

            __m512 x3 = _mm512_loadu_ps(&ps[k+3]);
            acc4 = _mm512_fmadd_ps(_mm512_set1_ps(coef0[-k-3]), x3, acc4);
            acc5 = _mm512_fmadd_ps(_mm512_set1_ps(coef1[-k-3]), x3, acc5);
            acc6 = _mm512_fmadd_ps(_mm512_set1_ps(coef2[-k-3]), x3, acc6);
            acc7 = _mm512_fmadd_ps(_mm512_set1_ps(coef3[-k-3]), x3, acc7);
        }

        acc0 = _mm512_add_ps(acc0, acc4);
        acc1 = _mm512_add_ps(acc1, acc5);
        acc2 = _mm512_add_ps(acc2, acc6);
        acc3 = _mm512_add_ps(acc3, acc7);

        _mm512_storeu_ps(&dst0[i], acc0);
        _mm512_storeu_ps(&dst1[i], acc1);
        _mm512_storeu_ps(&dst2[i], acc2);
        _mm512_storeu_ps(&dst3[i], acc3);
    }

    _mm256_zeroupper();
}

// FIXME: this fallback can be removed, once we require VS2017
#elif defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)

#include "../AudioHRTF.h"

void FIR_1x4_AVX2(float* src, float* dst0, float* dst1, float* dst2, float* dst3, float coef[4][HRTF_TAPS], int numFrames);

void FIR_1x4_AVX512(float* src, float* dst0, float* dst1, float* dst2, float* dst3, float coef[4][HRTF_TAPS], int numFrames) {
    FIR_1x4_AVX2(src, dst0, dst1, dst2, dst3, coef, numFrames);
}

#endif
