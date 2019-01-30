//
//  AudioHRTF_avx2.cpp
//  libraries/audio/src
//
//  Created by Ken Cooke on 1/17/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifdef __AVX2__

#include <assert.h>
#include <immintrin.h>

#include "../AudioHRTF.h"

#if defined(__GNUC__) && !defined(__clang__)
// for some reason, GCC -O2 results in poorly optimized code
#pragma GCC optimize("Os")
#endif

// 1 channel input, 4 channel output
void FIR_1x4_AVX2(float* src, float* dst0, float* dst1, float* dst2, float* dst3, float coef[4][HRTF_TAPS], int numFrames) {

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
        __m256 acc4 = _mm256_setzero_ps();
        __m256 acc5 = _mm256_setzero_ps();
        __m256 acc6 = _mm256_setzero_ps();
        __m256 acc7 = _mm256_setzero_ps();

        float* ps = &src[i - HRTF_TAPS + 1];    // process forwards

        static_assert(HRTF_TAPS % 4 == 0, "HRTF_TAPS must be a multiple of 4");

        for (int k = 0; k < HRTF_TAPS; k += 4) {

            __m256 x0 = _mm256_loadu_ps(&ps[k+0]);
            acc0 = _mm256_fmadd_ps(_mm256_broadcast_ss(&coef0[-k-0]), x0, acc0);
            acc1 = _mm256_fmadd_ps(_mm256_broadcast_ss(&coef1[-k-0]), x0, acc1);
            acc2 = _mm256_fmadd_ps(_mm256_broadcast_ss(&coef2[-k-0]), x0, acc2);
            acc3 = _mm256_fmadd_ps(_mm256_broadcast_ss(&coef3[-k-0]), x0, acc3);

            __m256 x1 = _mm256_loadu_ps(&ps[k+1]);
            acc4 = _mm256_fmadd_ps(_mm256_broadcast_ss(&coef0[-k-1]), x1, acc4);
            acc5 = _mm256_fmadd_ps(_mm256_broadcast_ss(&coef1[-k-1]), x1, acc5);
            acc6 = _mm256_fmadd_ps(_mm256_broadcast_ss(&coef2[-k-1]), x1, acc6);
            acc7 = _mm256_fmadd_ps(_mm256_broadcast_ss(&coef3[-k-1]), x1, acc7);

            __m256 x2 = _mm256_loadu_ps(&ps[k+2]);
            acc0 = _mm256_fmadd_ps(_mm256_broadcast_ss(&coef0[-k-2]), x2, acc0);
            acc1 = _mm256_fmadd_ps(_mm256_broadcast_ss(&coef1[-k-2]), x2, acc1);
            acc2 = _mm256_fmadd_ps(_mm256_broadcast_ss(&coef2[-k-2]), x2, acc2);
            acc3 = _mm256_fmadd_ps(_mm256_broadcast_ss(&coef3[-k-2]), x2, acc3);

            __m256 x3 = _mm256_loadu_ps(&ps[k+3]);
            acc4 = _mm256_fmadd_ps(_mm256_broadcast_ss(&coef0[-k-3]), x3, acc4);
            acc5 = _mm256_fmadd_ps(_mm256_broadcast_ss(&coef1[-k-3]), x3, acc5);
            acc6 = _mm256_fmadd_ps(_mm256_broadcast_ss(&coef2[-k-3]), x3, acc6);
            acc7 = _mm256_fmadd_ps(_mm256_broadcast_ss(&coef3[-k-3]), x3, acc7);
        }

        acc0 = _mm256_add_ps(acc0, acc4);
        acc1 = _mm256_add_ps(acc1, acc5);
        acc2 = _mm256_add_ps(acc2, acc6);
        acc3 = _mm256_add_ps(acc3, acc7);

        _mm256_storeu_ps(&dst0[i], acc0);
        _mm256_storeu_ps(&dst1[i], acc1);
        _mm256_storeu_ps(&dst2[i], acc2);
        _mm256_storeu_ps(&dst3[i], acc3);
    }

    _mm256_zeroupper();
}

// 4 channel planar to interleaved
void interleave_4x4_AVX2(float* src0, float* src1, float* src2, float* src3, float* dst, int numFrames) {

    assert(numFrames % 8 == 0);

    for (int i = 0; i < numFrames; i += 8) {

        __m256 x0 = _mm256_loadu_ps(&src0[i]);
        __m256 x1 = _mm256_loadu_ps(&src1[i]);
        __m256 x2 = _mm256_loadu_ps(&src2[i]);
        __m256 x3 = _mm256_loadu_ps(&src3[i]);

        // interleave (4x4 matrix transpose)
        __m256 t0 = _mm256_unpacklo_ps(x0, x1);
        __m256 t1 = _mm256_unpackhi_ps(x0, x1);
        __m256 t2 = _mm256_unpacklo_ps(x2, x3);
        __m256 t3 = _mm256_unpackhi_ps(x2, x3);

        x0 = _mm256_shuffle_ps(t0, t2, _MM_SHUFFLE(1,0,1,0));
        x1 = _mm256_shuffle_ps(t0, t2, _MM_SHUFFLE(3,2,3,2));
        x2 = _mm256_shuffle_ps(t1, t3, _MM_SHUFFLE(1,0,1,0));
        x3 = _mm256_shuffle_ps(t1, t3, _MM_SHUFFLE(3,2,3,2));

        t0 = _mm256_permute2f128_ps(x0, x1, 0x20);
        t1 = _mm256_permute2f128_ps(x2, x3, 0x20);
        t2 = _mm256_permute2f128_ps(x0, x1, 0x31);
        t3 = _mm256_permute2f128_ps(x2, x3, 0x31);

        _mm256_storeu_ps(&dst[4*i+0], t0);
        _mm256_storeu_ps(&dst[4*i+8], t1);
        _mm256_storeu_ps(&dst[4*i+16], t2);
        _mm256_storeu_ps(&dst[4*i+24], t3);
    }

    _mm256_zeroupper();
}

// process 2 cascaded biquads on 4 channels (interleaved)
// biquads are computed in parallel, by adding one sample of delay
void biquad2_4x4_AVX2(float* src, float* dst, float coef[5][8], float state[3][8], int numFrames) {

    // enable flush-to-zero mode to prevent denormals
    unsigned int ftz = _MM_GET_FLUSH_ZERO_MODE();
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);

    // restore state
    __m256 x0 = _mm256_setzero_ps();
    __m256 y0 = _mm256_loadu_ps(state[0]);
    __m256 w1 = _mm256_loadu_ps(state[1]);
    __m256 w2 = _mm256_loadu_ps(state[2]);

    //  biquad coefs
    __m256 b0 = _mm256_loadu_ps(coef[0]);
    __m256 b1 = _mm256_loadu_ps(coef[1]);
    __m256 b2 = _mm256_loadu_ps(coef[2]);
    __m256 a1 = _mm256_loadu_ps(coef[3]);
    __m256 a2 = _mm256_loadu_ps(coef[4]);

    for (int i = 0; i < numFrames; i++) {

        // x0 = (first biquad output << 128) | input
        x0 = _mm256_insertf128_ps(_mm256_permute2f128_ps(y0, y0, 0x01), _mm_loadu_ps(&src[4*i]), 0);

        // transposed Direct Form II
        y0 = _mm256_fmadd_ps(x0, b0, w1);
        w1 = _mm256_fmadd_ps(x0, b1, w2);
        w2 = _mm256_mul_ps(x0, b2);
        w1 = _mm256_fnmadd_ps(y0, a1, w1);
        w2 = _mm256_fnmadd_ps(y0, a2, w2);

        _mm_storeu_ps(&dst[4*i], _mm256_extractf128_ps(y0, 1)); // second biquad output
    }

    // save state
    _mm256_storeu_ps(state[0], y0);
    _mm256_storeu_ps(state[1], w1);
    _mm256_storeu_ps(state[2], w2);

    _MM_SET_FLUSH_ZERO_MODE(ftz);
    _mm256_zeroupper();
}

// crossfade 4 inputs into 2 outputs with accumulation (interleaved)
void crossfade_4x2_AVX2(float* src, float* dst, const float* win, int numFrames) {

    assert(numFrames % 8 == 0);

    for (int i = 0; i < numFrames; i += 8) {

        __m256 f0 = _mm256_loadu_ps(&win[i]);

        __m256 x0 = _mm256_castps128_ps256(_mm_loadu_ps(&src[4*i+0]));
        __m256 x1 = _mm256_castps128_ps256(_mm_loadu_ps(&src[4*i+4]));
        __m256 x2 = _mm256_castps128_ps256(_mm_loadu_ps(&src[4*i+8]));
        __m256 x3 = _mm256_castps128_ps256(_mm_loadu_ps(&src[4*i+12]));

        x0 = _mm256_insertf128_ps(x0, _mm_loadu_ps(&src[4*i+16]), 1);
        x1 = _mm256_insertf128_ps(x1, _mm_loadu_ps(&src[4*i+20]), 1);
        x2 = _mm256_insertf128_ps(x2, _mm_loadu_ps(&src[4*i+24]), 1);
        x3 = _mm256_insertf128_ps(x3, _mm_loadu_ps(&src[4*i+28]), 1);

        __m256 y0 = _mm256_loadu_ps(&dst[2*i+0]);
        __m256 y1 = _mm256_loadu_ps(&dst[2*i+8]);

        // deinterleave (4x4 matrix transpose)
        __m256 t0 = _mm256_unpacklo_ps(x0, x1);
        __m256 t1 = _mm256_unpackhi_ps(x0, x1);
        __m256 t2 = _mm256_unpacklo_ps(x2, x3);
        __m256 t3 = _mm256_unpackhi_ps(x2, x3);

        x0 = _mm256_shuffle_ps(t0, t2, _MM_SHUFFLE(1,0,1,0));
        x1 = _mm256_shuffle_ps(t0, t2, _MM_SHUFFLE(3,2,3,2));
        x2 = _mm256_shuffle_ps(t1, t3, _MM_SHUFFLE(1,0,1,0));
        x3 = _mm256_shuffle_ps(t1, t3, _MM_SHUFFLE(3,2,3,2));

        // crossfade
        x0 = _mm256_sub_ps(x0, x2);
        x1 = _mm256_sub_ps(x1, x3);
        x2 = _mm256_fmadd_ps(f0, x0, x2);
        x3 = _mm256_fmadd_ps(f0, x1, x3);

        // interleave
        t0 = _mm256_unpacklo_ps(x2, x3);
        t1 = _mm256_unpackhi_ps(x2, x3);

        x0 = _mm256_permute2f128_ps(t0, t1, 0x20);
        x1 = _mm256_permute2f128_ps(t0, t1, 0x31);

        // accumulate
        y0 = _mm256_add_ps(y0, x0);
        y1 = _mm256_add_ps(y1, x1);

        _mm256_storeu_ps(&dst[2*i+0], y0);
        _mm256_storeu_ps(&dst[2*i+8], y1);
    }

    _mm256_zeroupper();
}

// linear interpolation with gain
void interpolate_AVX2(const float* src0, const float* src1, float* dst, float frac, float gain) {

    __m256 f0 = _mm256_set1_ps(gain * (1.0f - frac));
    __m256 f1 = _mm256_set1_ps(gain * frac);

    static_assert(HRTF_TAPS % 8 == 0, "HRTF_TAPS must be a multiple of 8");

    for (int k = 0; k < HRTF_TAPS; k += 8) {

        __m256 x0 = _mm256_loadu_ps(&src0[k]);
        __m256 x1 = _mm256_loadu_ps(&src1[k]);

        x0 = _mm256_mul_ps(f0, x0);
        x0 = _mm256_fmadd_ps(f1, x1, x0);

        _mm256_storeu_ps(&dst[k], x0);
    }

    _mm256_zeroupper();
}

#endif
