//
//  AudioHRTF_avx.cpp
//  libraries/audio/src/avx
//
//  Created by Ken Cooke on 1/17/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)

#include <assert.h>
#include <immintrin.h>

#include "../AudioHRTF.h"

#ifndef __AVX__
#error Must be compiled with /arch:AVX or -mavx.
#endif

// 1 channel input, 4 channel output
void FIR_1x4_AVX(float* src, float* dst0, float* dst1, float* dst2, float* dst3, float coef[4][HRTF_TAPS], int numFrames) {

    float* coef0 = coef[0] + HRTF_TAPS - 1;     // process backwards
    float* coef1 = coef[1] + HRTF_TAPS - 1;
    float* coef2 = coef[2] + HRTF_TAPS - 1;
    float* coef3 = coef[3] + HRTF_TAPS - 1;

    assert(numFrames % 8 == 0);

    for (int i = 0; i < numFrames; i += 8) {

        __m256 acc0 = _mm256_setzero_ps();
        __m256 acc1 = _mm256_setzero_ps();
        __m256 acc2 = _mm256_setzero_ps();
        __m256 acc3 = _mm256_setzero_ps();

        float* ps = &src[i - HRTF_TAPS + 1];    // process forwards

        assert(HRTF_TAPS % 8 == 0);

        for (int k = 0; k < HRTF_TAPS; k += 8) {

            acc0 = _mm256_add_ps(acc0, _mm256_mul_ps(_mm256_broadcast_ss(&coef0[-k-0]), _mm256_loadu_ps(&ps[k+0])));
            acc1 = _mm256_add_ps(acc1, _mm256_mul_ps(_mm256_broadcast_ss(&coef1[-k-0]), _mm256_loadu_ps(&ps[k+0])));
            acc2 = _mm256_add_ps(acc2, _mm256_mul_ps(_mm256_broadcast_ss(&coef2[-k-0]), _mm256_loadu_ps(&ps[k+0])));
            acc3 = _mm256_add_ps(acc3, _mm256_mul_ps(_mm256_broadcast_ss(&coef3[-k-0]), _mm256_loadu_ps(&ps[k+0])));

            acc0 = _mm256_add_ps(acc0, _mm256_mul_ps(_mm256_broadcast_ss(&coef0[-k-1]), _mm256_loadu_ps(&ps[k+1])));
            acc1 = _mm256_add_ps(acc1, _mm256_mul_ps(_mm256_broadcast_ss(&coef1[-k-1]), _mm256_loadu_ps(&ps[k+1])));
            acc2 = _mm256_add_ps(acc2, _mm256_mul_ps(_mm256_broadcast_ss(&coef2[-k-1]), _mm256_loadu_ps(&ps[k+1])));
            acc3 = _mm256_add_ps(acc3, _mm256_mul_ps(_mm256_broadcast_ss(&coef3[-k-1]), _mm256_loadu_ps(&ps[k+1])));

            acc0 = _mm256_add_ps(acc0, _mm256_mul_ps(_mm256_broadcast_ss(&coef0[-k-2]), _mm256_loadu_ps(&ps[k+2])));
            acc1 = _mm256_add_ps(acc1, _mm256_mul_ps(_mm256_broadcast_ss(&coef1[-k-2]), _mm256_loadu_ps(&ps[k+2])));
            acc2 = _mm256_add_ps(acc2, _mm256_mul_ps(_mm256_broadcast_ss(&coef2[-k-2]), _mm256_loadu_ps(&ps[k+2])));
            acc3 = _mm256_add_ps(acc3, _mm256_mul_ps(_mm256_broadcast_ss(&coef3[-k-2]), _mm256_loadu_ps(&ps[k+2])));

            acc0 = _mm256_add_ps(acc0, _mm256_mul_ps(_mm256_broadcast_ss(&coef0[-k-3]), _mm256_loadu_ps(&ps[k+3])));
            acc1 = _mm256_add_ps(acc1, _mm256_mul_ps(_mm256_broadcast_ss(&coef1[-k-3]), _mm256_loadu_ps(&ps[k+3])));
            acc2 = _mm256_add_ps(acc2, _mm256_mul_ps(_mm256_broadcast_ss(&coef2[-k-3]), _mm256_loadu_ps(&ps[k+3])));
            acc3 = _mm256_add_ps(acc3, _mm256_mul_ps(_mm256_broadcast_ss(&coef3[-k-3]), _mm256_loadu_ps(&ps[k+3])));

            acc0 = _mm256_add_ps(acc0, _mm256_mul_ps(_mm256_broadcast_ss(&coef0[-k-4]), _mm256_loadu_ps(&ps[k+4])));
            acc1 = _mm256_add_ps(acc1, _mm256_mul_ps(_mm256_broadcast_ss(&coef1[-k-4]), _mm256_loadu_ps(&ps[k+4])));
            acc2 = _mm256_add_ps(acc2, _mm256_mul_ps(_mm256_broadcast_ss(&coef2[-k-4]), _mm256_loadu_ps(&ps[k+4])));
            acc3 = _mm256_add_ps(acc3, _mm256_mul_ps(_mm256_broadcast_ss(&coef3[-k-4]), _mm256_loadu_ps(&ps[k+4])));

            acc0 = _mm256_add_ps(acc0, _mm256_mul_ps(_mm256_broadcast_ss(&coef0[-k-5]), _mm256_loadu_ps(&ps[k+5])));
            acc1 = _mm256_add_ps(acc1, _mm256_mul_ps(_mm256_broadcast_ss(&coef1[-k-5]), _mm256_loadu_ps(&ps[k+5])));
            acc2 = _mm256_add_ps(acc2, _mm256_mul_ps(_mm256_broadcast_ss(&coef2[-k-5]), _mm256_loadu_ps(&ps[k+5])));
            acc3 = _mm256_add_ps(acc3, _mm256_mul_ps(_mm256_broadcast_ss(&coef3[-k-5]), _mm256_loadu_ps(&ps[k+5])));

            acc0 = _mm256_add_ps(acc0, _mm256_mul_ps(_mm256_broadcast_ss(&coef0[-k-6]), _mm256_loadu_ps(&ps[k+6])));
            acc1 = _mm256_add_ps(acc1, _mm256_mul_ps(_mm256_broadcast_ss(&coef1[-k-6]), _mm256_loadu_ps(&ps[k+6])));
            acc2 = _mm256_add_ps(acc2, _mm256_mul_ps(_mm256_broadcast_ss(&coef2[-k-6]), _mm256_loadu_ps(&ps[k+6])));
            acc3 = _mm256_add_ps(acc3, _mm256_mul_ps(_mm256_broadcast_ss(&coef3[-k-6]), _mm256_loadu_ps(&ps[k+6])));

            acc0 = _mm256_add_ps(acc0, _mm256_mul_ps(_mm256_broadcast_ss(&coef0[-k-7]), _mm256_loadu_ps(&ps[k+7])));
            acc1 = _mm256_add_ps(acc1, _mm256_mul_ps(_mm256_broadcast_ss(&coef1[-k-7]), _mm256_loadu_ps(&ps[k+7])));
            acc2 = _mm256_add_ps(acc2, _mm256_mul_ps(_mm256_broadcast_ss(&coef2[-k-7]), _mm256_loadu_ps(&ps[k+7])));
            acc3 = _mm256_add_ps(acc3, _mm256_mul_ps(_mm256_broadcast_ss(&coef3[-k-7]), _mm256_loadu_ps(&ps[k+7])));
        }

        _mm256_storeu_ps(&dst0[i], acc0);
        _mm256_storeu_ps(&dst1[i], acc1);
        _mm256_storeu_ps(&dst2[i], acc2);
        _mm256_storeu_ps(&dst3[i], acc3);
    }

    _mm256_zeroupper();
}

#endif
