//
//  CPUDetect.h
//  libraries/shared/src
//
//  Created by Ken Cooke on 6/6/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_CPUDetect_h
#define hifi_CPUDetect_h

//
// Lightweight functions to detect SSE/AVX/AVX2 support
//

#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)
#define ARCH_X86
#endif

#define MASK_SSE3   (1 << 0)                // SSE3
#define MASK_SSSE3  (1 << 9)                // SSSE3
#define MASK_SSE41  (1 << 19)               // SSE4.1
#define MASK_SSE42  ((1 << 20) | (1 << 23)) // SSE4.2 and POPCNT
#define MASK_AVX    ((1 << 27) | (1 << 28)) // OSXSAVE and AVX
#define MASK_AVX2   (1 << 5)                // AVX2

#if defined(ARCH_X86) && defined(_MSC_VER)

#include <intrin.h>

static inline bool cpuSupportsSSE3() {
    int info[4];

    __cpuidex(info, 0x1, 0);

    return ((info[2] & MASK_SSE3) == MASK_SSE3);
}

static inline bool cpuSupportsSSSE3() {
    int info[4];

    __cpuidex(info, 0x1, 0);

    return ((info[2] & MASK_SSSE3) == MASK_SSSE3);
}

static inline bool cpuSupportsSSE41() {
    int info[4];

    __cpuidex(info, 0x1, 0);

    return ((info[2] & MASK_SSE41) == MASK_SSE41);
}

static inline bool cpuSupportsSSE42() {
    int info[4];

    __cpuidex(info, 0x1, 0);

    return ((info[2] & MASK_SSE42) == MASK_SSE42);
}

static inline bool cpuSupportsAVX() {
    int info[4];

    __cpuidex(info, 0x1, 0);

    bool result = false;
    if ((info[2] & MASK_AVX) == MASK_AVX) {

        // verify OS support for YMM state
        if ((_xgetbv(_XCR_XFEATURE_ENABLED_MASK) & 0x6) == 0x6) {
            result = true;
        }
    }
    return result;
}

static inline bool cpuSupportsAVX2() {
    int info[4];

    bool result = false;
    if (cpuSupportsAVX()) {

        __cpuidex(info, 0x7, 0);

        if ((info[1] & MASK_AVX2) == MASK_AVX2) {
            result = true;
        }
    }
    return result;
}

#elif defined(ARCH_X86) && defined(__GNUC__)

#include <cpuid.h>

static inline bool cpuSupportsSSE3() {
    unsigned int eax, ebx, ecx, edx;

    return __get_cpuid(0x1, &eax, &ebx, &ecx, &edx) && ((ecx & MASK_SSE3) == MASK_SSE3);
}

static inline bool cpuSupportsSSSE3() {
    unsigned int eax, ebx, ecx, edx;

    return __get_cpuid(0x1, &eax, &ebx, &ecx, &edx) && ((ecx & MASK_SSSE3) == MASK_SSSE3);
}

static inline bool cpuSupportsSSE41() {
    unsigned int eax, ebx, ecx, edx;

    return __get_cpuid(0x1, &eax, &ebx, &ecx, &edx) && ((ecx & MASK_SSE41) == MASK_SSE41);
}

static inline bool cpuSupportsSSE42() {
    unsigned int eax, ebx, ecx, edx;

    return __get_cpuid(0x1, &eax, &ebx, &ecx, &edx) && ((ecx & MASK_SSE42) == MASK_SSE42);
}

static inline bool cpuSupportsAVX() {
    unsigned int eax, ebx, ecx, edx;

    bool result = false;
    if (__get_cpuid(0x1, &eax, &ebx, &ecx, &edx) && ((ecx & MASK_AVX) == MASK_AVX)) {

        // verify OS support for YMM state
        __asm__("xgetbv" : "=a"(eax), "=d"(edx) : "c"(0));
        if ((eax & 0x6) == 0x6) {
            result = true;
        }
    }
    return result;    
}

static inline bool cpuSupportsAVX2() {
    unsigned int eax, ebx, ecx, edx;

    bool result = false;
    if (cpuSupportsAVX()) {

        if (__get_cpuid(0x7, &eax, &ebx, &ecx, &edx) && ((ebx & MASK_AVX2) == MASK_AVX2)) {
            result = true;
        }
    }
    return result;    
}

#else

static inline bool cpuSupportsSSE3() {
    return false;
}

static inline bool cpuSupportsSSSE3() {
    return false;
}

static inline bool cpuSupportsSSE41() {
    return false;
}

static inline bool cpuSupportsSSE42() {
    return false;
}

static inline bool cpuSupportsAVX() {
    return false;
}

static inline bool cpuSupportsAVX2() {
    return false;
}

#endif

#endif // hifi_CPUDetect_h
