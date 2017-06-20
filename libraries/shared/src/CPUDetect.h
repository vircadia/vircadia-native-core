//
//  CPUDetect.h
//  libraries/shared/src
//
//  Created by Ken Cooke on 6/16/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_CPUDetect_h
#define hifi_CPUDetect_h

//
// Lightweight functions to detect SSE/AVX/AVX2/AVX512 support
//

#define MASK_SSE3       (1 << 0)                // SSE3
#define MASK_SSSE3      (1 << 9)                // SSSE3
#define MASK_SSE41      (1 << 19)               // SSE4.1
#define MASK_SSE42      ((1 << 20) | (1 << 23)) // SSE4.2 and POPCNT
#define MASK_OSXSAVE    (1 << 27)               // OSXSAVE
#define MASK_AVX        ((1 << 27) | (1 << 28)) // OSXSAVE and AVX
#define MASK_AVX2       (1 << 5)                // AVX2

#define MASK_AVX512     ((1 << 16) | (1 << 17) | (1 << 28) | (1 << 30) | (1 << 31)) // AVX512 F,DQ,CD,BW,VL (SKX)

#define MASK_XCR0_YMM   ((1 << 1) | (1 << 2))               // XMM,YMM
#define MASK_XCR0_ZMM   ((1 << 1) | (1 << 2) | (7 << 5))    // XMM,YMM,ZMM

#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)
#define ARCH_X86
#endif

#if defined(ARCH_X86) && defined(_MSC_VER)

#include <intrin.h>

// use MSVC intrinsics
#define cpuidex(info, eax, ecx)     __cpuidex(info, eax, ecx)
#define xgetbv(ecx)                 _xgetbv(ecx)

#elif defined(ARCH_X86) && defined(__GNUC__)

#include <cpuid.h>

// use GCC intrinics/asm
static inline void cpuidex(int info[4], int eax, int ecx) {
    __cpuid_count(eax, ecx, info[0], info[1], info[2], info[3]);
}

static inline unsigned long long xgetbv(unsigned int ecx){
    unsigned int eax, edx;
    __asm__("xgetbv" : "=a"(eax), "=d"(edx) : "c"(ecx));
    return ((unsigned long long)edx << 32) | eax;
}

#else

static inline void cpuidex(int info[4], int eax, int ecx) {
    info[0] = 0;
    info[1] = 0;
    info[2] = 0;
    info[3] = 0;
}

static inline unsigned long long xgetbv(unsigned int ecx){
    return 0ULL;
}

#endif

static inline bool cpuSupportsSSE3() {
    int info[4];

    cpuidex(info, 0x1, 0);

    return ((info[2] & MASK_SSE3) == MASK_SSE3);
}

static inline bool cpuSupportsSSSE3() {
    int info[4];

    cpuidex(info, 0x1, 0);

    return ((info[2] & MASK_SSSE3) == MASK_SSSE3);
}

static inline bool cpuSupportsSSE41() {
    int info[4];

    cpuidex(info, 0x1, 0);

    return ((info[2] & MASK_SSE41) == MASK_SSE41);
}

static inline bool cpuSupportsSSE42() {
    int info[4];

    cpuidex(info, 0x1, 0);

    return ((info[2] & MASK_SSE42) == MASK_SSE42);
}

static inline bool cpuSupportsAVX() {
    int info[4];

    cpuidex(info, 0x1, 0);

    bool result = false;
    if ((info[2] & MASK_AVX) == MASK_AVX) {

        // verify OS support for YMM state
        if ((xgetbv(0) & MASK_XCR0_YMM) == MASK_XCR0_YMM) {
            result = true;
        }
    }
    return result;
}

static inline bool cpuSupportsAVX2() {
    int info[4];

    bool result = false;
    if (cpuSupportsAVX()) {

        cpuidex(info, 0x7, 0);

        if ((info[1] & MASK_AVX2) == MASK_AVX2) {
            result = true;
        }
    }
    return result;
}

static inline bool cpuSupportsAVX512() {
    int info[4];

    cpuidex(info, 0x1, 0);

    bool result = false;
    if ((info[2] & MASK_OSXSAVE) == MASK_OSXSAVE) {

        // verify OS support for ZMM state
        if ((xgetbv(0) & MASK_XCR0_ZMM) == MASK_XCR0_ZMM) {

            cpuidex(info, 0x7, 0);

            if ((info[1] & MASK_AVX512) == MASK_AVX512) {
                result = true;
            }
        }
    }
    return result;
}

#endif // hifi_CPUDetect_h
