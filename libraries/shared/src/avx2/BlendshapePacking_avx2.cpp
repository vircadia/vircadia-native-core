//
//  BlendshapePacking_avx2.cpp
//
//  Created by Ken Cooke on 6/22/19.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifdef __AVX2__

#include <stdint.h>
#include <immintrin.h>

void packBlendshapeOffsets_AVX2(float (*unpacked)[9], uint32_t (*packed)[4], int size) {
    
    int i = 0;
    for (; i < size - 7; i += 8) {  // blocks of 8

        //
        // deinterleave (8x9 to 9x8 matrix transpose)
        //
        __m256 s0 = _mm256_insertf128_ps(_mm256_castps128_ps256(_mm_loadu_ps(&unpacked[i+0][0])), _mm_loadu_ps(&unpacked[i+4][0]), 1);
        __m256 s1 = _mm256_insertf128_ps(_mm256_castps128_ps256(_mm_loadu_ps(&unpacked[i+1][0])), _mm_loadu_ps(&unpacked[i+5][0]), 1);
        __m256 s2 = _mm256_insertf128_ps(_mm256_castps128_ps256(_mm_loadu_ps(&unpacked[i+2][0])), _mm_loadu_ps(&unpacked[i+6][0]), 1);
        __m256 s3 = _mm256_insertf128_ps(_mm256_castps128_ps256(_mm_loadu_ps(&unpacked[i+3][0])), _mm_loadu_ps(&unpacked[i+7][0]), 1);
        __m256 s4 = _mm256_insertf128_ps(_mm256_castps128_ps256(_mm_loadu_ps(&unpacked[i+0][4])), _mm_loadu_ps(&unpacked[i+4][4]), 1);
        __m256 s5 = _mm256_insertf128_ps(_mm256_castps128_ps256(_mm_loadu_ps(&unpacked[i+1][4])), _mm_loadu_ps(&unpacked[i+5][4]), 1);
        __m256 s6 = _mm256_insertf128_ps(_mm256_castps128_ps256(_mm_loadu_ps(&unpacked[i+2][4])), _mm_loadu_ps(&unpacked[i+6][4]), 1);
        __m256 s7 = _mm256_insertf128_ps(_mm256_castps128_ps256(_mm_loadu_ps(&unpacked[i+3][4])), _mm_loadu_ps(&unpacked[i+7][4]), 1);

        __m256 t0 = _mm256_unpacklo_ps(s0, s1);
        __m256 t1 = _mm256_unpackhi_ps(s0, s1);
        __m256 t2 = _mm256_unpacklo_ps(s2, s3);
        __m256 t3 = _mm256_unpackhi_ps(s2, s3);
        __m256 t4 = _mm256_unpacklo_ps(s4, s5);
        __m256 t5 = _mm256_unpackhi_ps(s4, s5);
        __m256 t6 = _mm256_unpacklo_ps(s6, s7);
        __m256 t7 = _mm256_unpackhi_ps(s6, s7);

        __m256 px = _mm256_shuffle_ps(t0, t2, _MM_SHUFFLE(1,0,1,0));
        __m256 py = _mm256_shuffle_ps(t0, t2, _MM_SHUFFLE(3,2,3,2));
        __m256 pz = _mm256_shuffle_ps(t1, t3, _MM_SHUFFLE(1,0,1,0));
        __m256 nx = _mm256_shuffle_ps(t1, t3, _MM_SHUFFLE(3,2,3,2));
        __m256 ny = _mm256_shuffle_ps(t4, t6, _MM_SHUFFLE(1,0,1,0));
        __m256 nz = _mm256_shuffle_ps(t4, t6, _MM_SHUFFLE(3,2,3,2));
        __m256 tx = _mm256_shuffle_ps(t5, t7, _MM_SHUFFLE(1,0,1,0));
        __m256 ty = _mm256_shuffle_ps(t5, t7, _MM_SHUFFLE(3,2,3,2));

        __m256 tz = _mm256_i32gather_ps(unpacked[i+0], _mm256_setr_epi32(8,17,26,35,44,53,62,71), sizeof(float));

        // abs(pos)
        __m256 apx = _mm256_andnot_ps(_mm256_set1_ps(-0.0f), px);
        __m256 apy = _mm256_andnot_ps(_mm256_set1_ps(-0.0f), py);
        __m256 apz = _mm256_andnot_ps(_mm256_set1_ps(-0.0f), pz);

        // len = compMax(abs(pos))
        __m256 len = _mm256_max_ps(_mm256_max_ps(apx, apy), apz);

        // detect zeros
        __m256 mask = _mm256_cmp_ps(len, _mm256_setzero_ps(), _CMP_EQ_OQ);

        // rcp = 1.0f / len
        __m256 rcp = _mm256_div_ps(_mm256_set1_ps(1.0f), len);

        // replace +inf with 1.0f
        rcp = _mm256_blendv_ps(rcp, _mm256_set1_ps(1.0f), mask);
        len = _mm256_blendv_ps(len, _mm256_set1_ps(1.0f), mask);

        // pos *= 1.0f / len
        px = _mm256_mul_ps(px, rcp);
        py = _mm256_mul_ps(py, rcp);
        pz = _mm256_mul_ps(pz, rcp);

        // clamp(vec, -1.0f, 1.0f)
        px = _mm256_min_ps(_mm256_max_ps(px, _mm256_set1_ps(-1.0f)), _mm256_set1_ps(1.0f));
        py = _mm256_min_ps(_mm256_max_ps(py, _mm256_set1_ps(-1.0f)), _mm256_set1_ps(1.0f));
        pz = _mm256_min_ps(_mm256_max_ps(pz, _mm256_set1_ps(-1.0f)), _mm256_set1_ps(1.0f));
        nx = _mm256_min_ps(_mm256_max_ps(nx, _mm256_set1_ps(-1.0f)), _mm256_set1_ps(1.0f));
        ny = _mm256_min_ps(_mm256_max_ps(ny, _mm256_set1_ps(-1.0f)), _mm256_set1_ps(1.0f));
        nz = _mm256_min_ps(_mm256_max_ps(nz, _mm256_set1_ps(-1.0f)), _mm256_set1_ps(1.0f));
        tx = _mm256_min_ps(_mm256_max_ps(tx, _mm256_set1_ps(-1.0f)), _mm256_set1_ps(1.0f));
        ty = _mm256_min_ps(_mm256_max_ps(ty, _mm256_set1_ps(-1.0f)), _mm256_set1_ps(1.0f));
        tz = _mm256_min_ps(_mm256_max_ps(tz, _mm256_set1_ps(-1.0f)), _mm256_set1_ps(1.0f));

        // vec *= 511.0f
        px = _mm256_mul_ps(px, _mm256_set1_ps(511.0f));
        py = _mm256_mul_ps(py, _mm256_set1_ps(511.0f));
        pz = _mm256_mul_ps(pz, _mm256_set1_ps(511.0f));
        nx = _mm256_mul_ps(nx, _mm256_set1_ps(511.0f));
        ny = _mm256_mul_ps(ny, _mm256_set1_ps(511.0f));
        nz = _mm256_mul_ps(nz, _mm256_set1_ps(511.0f));
        tx = _mm256_mul_ps(tx, _mm256_set1_ps(511.0f));
        ty = _mm256_mul_ps(ty, _mm256_set1_ps(511.0f));
        tz = _mm256_mul_ps(tz, _mm256_set1_ps(511.0f));

        // veci = lrint(vec) & 03ff
        __m256i pxi = _mm256_and_si256(_mm256_cvtps_epi32(px), _mm256_set1_epi32(0x3ff));
        __m256i pyi = _mm256_and_si256(_mm256_cvtps_epi32(py), _mm256_set1_epi32(0x3ff));
        __m256i pzi = _mm256_and_si256(_mm256_cvtps_epi32(pz), _mm256_set1_epi32(0x3ff));
        __m256i nxi = _mm256_and_si256(_mm256_cvtps_epi32(nx), _mm256_set1_epi32(0x3ff));
        __m256i nyi = _mm256_and_si256(_mm256_cvtps_epi32(ny), _mm256_set1_epi32(0x3ff));
        __m256i nzi = _mm256_and_si256(_mm256_cvtps_epi32(nz), _mm256_set1_epi32(0x3ff));
        __m256i txi = _mm256_and_si256(_mm256_cvtps_epi32(tx), _mm256_set1_epi32(0x3ff));
        __m256i tyi = _mm256_and_si256(_mm256_cvtps_epi32(ty), _mm256_set1_epi32(0x3ff));
        __m256i tzi = _mm256_and_si256(_mm256_cvtps_epi32(tz), _mm256_set1_epi32(0x3ff));

        // pack = (xi << 0) | (yi << 10) | (zi << 20);
        __m256i li = _mm256_castps_si256(len);                                                                      // length
        __m256i pi = _mm256_or_si256(_mm256_or_si256(pxi, _mm256_slli_epi32(pyi, 10)), _mm256_slli_epi32(pzi, 20)); // position
        __m256i ni = _mm256_or_si256(_mm256_or_si256(nxi, _mm256_slli_epi32(nyi, 10)), _mm256_slli_epi32(nzi, 20)); // normal
        __m256i ti = _mm256_or_si256(_mm256_or_si256(txi, _mm256_slli_epi32(tyi, 10)), _mm256_slli_epi32(tzi, 20)); // tangent

        //
        // interleave (4x4 matrix transpose)
        //
        __m256i u0 = _mm256_unpacklo_epi32(li, pi);
        __m256i u1 = _mm256_unpackhi_epi32(li, pi);
        __m256i u2 = _mm256_unpacklo_epi32(ni, ti);
        __m256i u3 = _mm256_unpackhi_epi32(ni, ti);

        __m256i v0 = _mm256_unpacklo_epi64(u0, u2);
        __m256i v1 = _mm256_unpackhi_epi64(u0, u2);
        __m256i v2 = _mm256_unpacklo_epi64(u1, u3);
        __m256i v3 = _mm256_unpackhi_epi64(u1, u3);

        __m256i w0 = _mm256_permute2f128_si256(v0, v1, 0x20);
        __m256i w1 = _mm256_permute2f128_si256(v2, v3, 0x20);
        __m256i w2 = _mm256_permute2f128_si256(v0, v1, 0x31);
        __m256i w3 = _mm256_permute2f128_si256(v2, v3, 0x31);

        // store pack x 8
        _mm256_storeu_si256((__m256i*)packed[i+0], w0);
        _mm256_storeu_si256((__m256i*)packed[i+2], w1);
        _mm256_storeu_si256((__m256i*)packed[i+4], w2);
        _mm256_storeu_si256((__m256i*)packed[i+6], w3);
    }

    if (i < size) { // remainder
        int rem = size - i;
        
        //
        // deinterleave (8x9 to 9x8 matrix transpose)
        //
        __m256 s0 = _mm256_setzero_ps();
        __m256 s1 = _mm256_setzero_ps();
        __m256 s2 = _mm256_setzero_ps();
        __m256 s3 = _mm256_setzero_ps();
        __m256 s4 = _mm256_setzero_ps();
        __m256 s5 = _mm256_setzero_ps();
        __m256 s6 = _mm256_setzero_ps();
        __m256 s7 = _mm256_setzero_ps();

        switch (rem) {
            case 7: s6 = _mm256_loadu_ps(unpacked[i+6]); /* fall-thru */
            case 6: s5 = _mm256_loadu_ps(unpacked[i+5]); /* fall-thru */
            case 5: s4 = _mm256_loadu_ps(unpacked[i+4]); /* fall-thru */
            case 4: s3 = _mm256_loadu_ps(unpacked[i+3]); /* fall-thru */
            case 3: s2 = _mm256_loadu_ps(unpacked[i+2]); /* fall-thru */
            case 2: s1 = _mm256_loadu_ps(unpacked[i+1]); /* fall-thru */
            case 1: s0 = _mm256_loadu_ps(unpacked[i+0]); /* fall-thru */
        }
        
        __m256 t0 = _mm256_unpacklo_ps(s0, s1);
        __m256 t1 = _mm256_unpackhi_ps(s0, s1);
        __m256 t2 = _mm256_unpacklo_ps(s2, s3);
        __m256 t3 = _mm256_unpackhi_ps(s2, s3);
        __m256 t4 = _mm256_unpacklo_ps(s4, s5);
        __m256 t5 = _mm256_unpackhi_ps(s4, s5);
        __m256 t6 = _mm256_unpacklo_ps(s6, s7);
        __m256 t7 = _mm256_unpackhi_ps(s6, s7);

        s0 = _mm256_shuffle_ps(t0, t2, _MM_SHUFFLE(1,0,1,0));
        s1 = _mm256_shuffle_ps(t0, t2, _MM_SHUFFLE(3,2,3,2));
        s2 = _mm256_shuffle_ps(t1, t3, _MM_SHUFFLE(1,0,1,0));
        s3 = _mm256_shuffle_ps(t1, t3, _MM_SHUFFLE(3,2,3,2));
        s4 = _mm256_shuffle_ps(t4, t6, _MM_SHUFFLE(1,0,1,0));
        s5 = _mm256_shuffle_ps(t4, t6, _MM_SHUFFLE(3,2,3,2));
        s6 = _mm256_shuffle_ps(t5, t7, _MM_SHUFFLE(1,0,1,0));
        s7 = _mm256_shuffle_ps(t5, t7, _MM_SHUFFLE(3,2,3,2));

        __m256 px = _mm256_permute2f128_ps(s0, s4, 0x20);
        __m256 py = _mm256_permute2f128_ps(s1, s5, 0x20);
        __m256 pz = _mm256_permute2f128_ps(s2, s6, 0x20);
        __m256 nx = _mm256_permute2f128_ps(s3, s7, 0x20);
        __m256 ny = _mm256_permute2f128_ps(s0, s4, 0x31);
        __m256 nz = _mm256_permute2f128_ps(s1, s5, 0x31);
        __m256 tx = _mm256_permute2f128_ps(s2, s6, 0x31);
        __m256 ty = _mm256_permute2f128_ps(s3, s7, 0x31);

        __m256i loadmask = _mm256_cvtepi8_epi32(_mm_cvtsi64_si128(0xffffffffffffffffULL >> (64 - 8 * rem)));
        __m256 tz = _mm256_mask_i32gather_ps(_mm256_setzero_ps(), unpacked[i+0], _mm256_setr_epi32(8,17,26,35,44,53,62,71), 
                                             _mm256_castsi256_ps(loadmask), sizeof(float));
        // abs(pos)
        __m256 apx = _mm256_andnot_ps(_mm256_set1_ps(-0.0f), px);
        __m256 apy = _mm256_andnot_ps(_mm256_set1_ps(-0.0f), py);
        __m256 apz = _mm256_andnot_ps(_mm256_set1_ps(-0.0f), pz);

        // len = compMax(abs(pos))
        __m256 len = _mm256_max_ps(_mm256_max_ps(apx, apy), apz);

        // detect zeros
        __m256 mask = _mm256_cmp_ps(len, _mm256_setzero_ps(), _CMP_EQ_OQ);

        // rcp = 1.0f / len
        __m256 rcp = _mm256_div_ps(_mm256_set1_ps(1.0f), len);

        // replace +inf with 1.0f
        rcp = _mm256_blendv_ps(rcp, _mm256_set1_ps(1.0f), mask);
        len = _mm256_blendv_ps(len, _mm256_set1_ps(1.0f), mask);

        // pos *= 1.0f / len
        px = _mm256_mul_ps(px, rcp);
        py = _mm256_mul_ps(py, rcp);
        pz = _mm256_mul_ps(pz, rcp);

        // clamp(vec, -1.0f, 1.0f)
        px = _mm256_min_ps(_mm256_max_ps(px, _mm256_set1_ps(-1.0f)), _mm256_set1_ps(1.0f));
        py = _mm256_min_ps(_mm256_max_ps(py, _mm256_set1_ps(-1.0f)), _mm256_set1_ps(1.0f));
        pz = _mm256_min_ps(_mm256_max_ps(pz, _mm256_set1_ps(-1.0f)), _mm256_set1_ps(1.0f));
        nx = _mm256_min_ps(_mm256_max_ps(nx, _mm256_set1_ps(-1.0f)), _mm256_set1_ps(1.0f));
        ny = _mm256_min_ps(_mm256_max_ps(ny, _mm256_set1_ps(-1.0f)), _mm256_set1_ps(1.0f));
        nz = _mm256_min_ps(_mm256_max_ps(nz, _mm256_set1_ps(-1.0f)), _mm256_set1_ps(1.0f));
        tx = _mm256_min_ps(_mm256_max_ps(tx, _mm256_set1_ps(-1.0f)), _mm256_set1_ps(1.0f));
        ty = _mm256_min_ps(_mm256_max_ps(ty, _mm256_set1_ps(-1.0f)), _mm256_set1_ps(1.0f));
        tz = _mm256_min_ps(_mm256_max_ps(tz, _mm256_set1_ps(-1.0f)), _mm256_set1_ps(1.0f));

        // vec *= 511.0f
        px = _mm256_mul_ps(px, _mm256_set1_ps(511.0f));
        py = _mm256_mul_ps(py, _mm256_set1_ps(511.0f));
        pz = _mm256_mul_ps(pz, _mm256_set1_ps(511.0f));
        nx = _mm256_mul_ps(nx, _mm256_set1_ps(511.0f));
        ny = _mm256_mul_ps(ny, _mm256_set1_ps(511.0f));
        nz = _mm256_mul_ps(nz, _mm256_set1_ps(511.0f));
        tx = _mm256_mul_ps(tx, _mm256_set1_ps(511.0f));
        ty = _mm256_mul_ps(ty, _mm256_set1_ps(511.0f));
        tz = _mm256_mul_ps(tz, _mm256_set1_ps(511.0f));

        // veci = lrint(vec) & 03ff
        __m256i pxi = _mm256_and_si256(_mm256_cvtps_epi32(px), _mm256_set1_epi32(0x3ff));
        __m256i pyi = _mm256_and_si256(_mm256_cvtps_epi32(py), _mm256_set1_epi32(0x3ff));
        __m256i pzi = _mm256_and_si256(_mm256_cvtps_epi32(pz), _mm256_set1_epi32(0x3ff));
        __m256i nxi = _mm256_and_si256(_mm256_cvtps_epi32(nx), _mm256_set1_epi32(0x3ff));
        __m256i nyi = _mm256_and_si256(_mm256_cvtps_epi32(ny), _mm256_set1_epi32(0x3ff));
        __m256i nzi = _mm256_and_si256(_mm256_cvtps_epi32(nz), _mm256_set1_epi32(0x3ff));
        __m256i txi = _mm256_and_si256(_mm256_cvtps_epi32(tx), _mm256_set1_epi32(0x3ff));
        __m256i tyi = _mm256_and_si256(_mm256_cvtps_epi32(ty), _mm256_set1_epi32(0x3ff));
        __m256i tzi = _mm256_and_si256(_mm256_cvtps_epi32(tz), _mm256_set1_epi32(0x3ff));

        // pack = (xi << 0) | (yi << 10) | (zi << 20);
        __m256i li = _mm256_castps_si256(len);                                                                      // length
        __m256i pi = _mm256_or_si256(_mm256_or_si256(pxi, _mm256_slli_epi32(pyi, 10)), _mm256_slli_epi32(pzi, 20)); // position
        __m256i ni = _mm256_or_si256(_mm256_or_si256(nxi, _mm256_slli_epi32(nyi, 10)), _mm256_slli_epi32(nzi, 20)); // normal
        __m256i ti = _mm256_or_si256(_mm256_or_si256(txi, _mm256_slli_epi32(tyi, 10)), _mm256_slli_epi32(tzi, 20)); // tangent

        //
        // interleave (4x4 matrix transpose)
        //
        __m256i u0 = _mm256_unpacklo_epi32(li, pi);
        __m256i u1 = _mm256_unpackhi_epi32(li, pi);
        __m256i u2 = _mm256_unpacklo_epi32(ni, ti);
        __m256i u3 = _mm256_unpackhi_epi32(ni, ti);

        __m256i v0 = _mm256_unpacklo_epi64(u0, u2);
        __m256i v1 = _mm256_unpackhi_epi64(u0, u2);
        __m256i v2 = _mm256_unpacklo_epi64(u1, u3);
        __m256i v3 = _mm256_unpackhi_epi64(u1, u3);

        // store pack x 8
        switch (rem) {
            case 7: _mm_storeu_si128((__m128i*)packed[i+6], _mm256_extractf128_si256(v2, 1)); /* fall-thru */
            case 6: _mm_storeu_si128((__m128i*)packed[i+5], _mm256_extractf128_si256(v1, 1)); /* fall-thru */
            case 5: _mm_storeu_si128((__m128i*)packed[i+4], _mm256_extractf128_si256(v0, 1)); /* fall-thru */
            case 4: _mm_storeu_si128((__m128i*)packed[i+3], _mm256_castsi256_si128(v3)); /* fall-thru */
            case 3: _mm_storeu_si128((__m128i*)packed[i+2], _mm256_castsi256_si128(v2)); /* fall-thru */
            case 2: _mm_storeu_si128((__m128i*)packed[i+1], _mm256_castsi256_si128(v1)); /* fall-thru */
            case 1: _mm_storeu_si128((__m128i*)packed[i+0], _mm256_castsi256_si128(v0)); /* fall-thru */
        }
    }

    _mm256_zeroupper();
}

#endif
