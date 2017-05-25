//
//  AudioDynamics.h
//  libraries/audio/src
//
//  Created by Ken Cooke on 5/5/17.
//  Copyright 2017 High Fidelity, Inc.
//

//
// Inline functions to implement audio dynamics processing
//

#include <math.h>
#include <stdint.h>

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif 
#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifdef _MSC_VER
#include <intrin.h>
#define MUL64(a,b)  __emul((a), (b))
#else
#define MUL64(a,b)  ((int64_t)(a) * (int64_t)(b))
#endif

#define MULHI(a,b)      ((int32_t)(MUL64(a, b) >> 32))
#define MULQ31(a,b)     ((int32_t)(MUL64(a, b) >> 31))
#define MULDIV64(a,b,c) (int32_t)(MUL64(a, b) / (c))

//
// on x86 architecture, assume that SSE2 is present
//
#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)

#include <xmmintrin.h>
// convert float to int using round-to-nearest
static inline int32_t floatToInt(float x) {
    return _mm_cvt_ss2si(_mm_load_ss(&x));
}

#else 

// convert float to int using round-to-nearest
static inline int32_t floatToInt(float x) {
    x += (x < 0.0f ? -0.5f : 0.5f); // round
    return (int32_t)x;
}

#endif  // _M_IX86

static const double FIXQ31 = 2147483648.0;              // convert float to Q31
static const double DB_TO_LOG2 = 0.16609640474436813;   // convert dB to log2

// convert dB to amplitude
static inline double dBToGain(double dB) {
    return pow(10.0, dB / 20.0);
}

// convert milliseconds to first-order time constant
static inline int32_t msToTc(double ms, double sampleRate) {
    double tc = exp(-1000.0 / (ms * sampleRate));
    return (int32_t)(FIXQ31 * tc);  // Q31
}

// log2 domain values are Q26
static const int LOG2_INTBITS = 5;
static const int LOG2_FRACBITS = 31 - LOG2_INTBITS;

// log2 domain headroom bits above 0dB
static const int LOG2_HEADROOM = 15;

// log2 domain offsets so error < 0
static const int32_t LOG2_BIAS = 347;
static const int32_t EXP2_BIAS = 64;

//
// P(x) = log2(1+x) for x=[0,1]
// scaled by 1, 0.5, 0.25
//
// |error| < 347 ulp, smooth
//
static const int LOG2_TABBITS = 4;
static const int32_t log2Table[1 << LOG2_TABBITS][3] = {
    { -0x56dfe26d, 0x5c46daff, 0x00000000 },
    { -0x4d397571, 0x5bae58e7, 0x00025a75 },
    { -0x4518f84b, 0x5aabcac4, 0x000a62db },
    { -0x3e3075ec, 0x596168c0, 0x0019d0e6 },
    { -0x384486e9, 0x57e769c7, 0x00316109 },
    { -0x332742ba, 0x564f1461, 0x00513776 },
    { -0x2eb4bad4, 0x54a4cdfe, 0x00791de2 },
    { -0x2ad07c6c, 0x52f18320, 0x00a8aa46 },
    { -0x2763c4d6, 0x513ba123, 0x00df574c },
    { -0x245c319b, 0x4f87c5c4, 0x011c9399 },
    { -0x21aac79f, 0x4dd93bef, 0x015fcb52 },
    { -0x1f433872, 0x4c325584, 0x01a86ddc },
    { -0x1d1b54b4, 0x4a94ac6e, 0x01f5f13e },
    { -0x1b2a9f81, 0x4901524f, 0x0247d3f2 },
    { -0x1969fa57, 0x4778f3a7, 0x029d9dbf },
    { -0x17d36370, 0x45fbf1e8, 0x02f6dfe8 },
};

//
// P(x) = exp2(x) for x=[0,1]
// scaled by 2, 1, 0.5
// Uses exp2(-x) = exp2(1-x)/2
//
// |error| < 1387 ulp, smooth
//
static const int EXP2_TABBITS = 4;
static const int32_t exp2Table[1 << EXP2_TABBITS][3] = {
    { 0x3ed838c8, 0x58b574b7, 0x40000000 },
    { 0x41a0821c, 0x5888db8f, 0x4000b2b7 },
    { 0x4488548d, 0x582bcbc6, 0x40039be1 },
    { 0x4791158a, 0x579a1128, 0x400a71ae },
    { 0x4abc3a53, 0x56cf3089, 0x4017212e },
    { 0x4e0b48af, 0x55c66396, 0x402bd31b },
    { 0x517fd7a7, 0x547a946d, 0x404af0ec },
    { 0x551b9049, 0x52e658f9, 0x40772a57 },
    { 0x58e02e75, 0x5103ee08, 0x40b37b31 },
    { 0x5ccf81b1, 0x4ecd321f, 0x410331b5 },
    { 0x60eb6e09, 0x4c3ba007, 0x4169f548 },
    { 0x6535ecf9, 0x49484909, 0x41ebcdaf },
    { 0x69b10e5b, 0x45ebcede, 0x428d2acd },
    { 0x6e5ef96c, 0x421e5d48, 0x4352ece7 },
    { 0x7341edcb, 0x3dd7a354, 0x44426d7b },
    { 0x785c4499, 0x390ecc3a, 0x456188bd },
};

static const int IEEE754_FABS_MASK = 0x7fffffff;
static const int IEEE754_MANT_BITS = 23;
static const int IEEE754_EXPN_BIAS = 127;

//
// Peak detection and -log2(x) for float input (mono)
// x < 2^(31-LOG2_HEADROOM) returns 0x7fffffff
// x > 2^LOG2_HEADROOM undefined
//
static inline int32_t peaklog2(float* input) {

    // float as integer bits
    int32_t u = *(int32_t*)input;

    // absolute value
    int32_t peak = u & IEEE754_FABS_MASK;

    // split into e and x - 1.0
    int32_t e = IEEE754_EXPN_BIAS - (peak >> IEEE754_MANT_BITS) + LOG2_HEADROOM;
    int32_t x = (peak << (31 - IEEE754_MANT_BITS)) & 0x7fffffff;

    // saturate
    if (e > 31) {
        return 0x7fffffff;
    }

    int k = x >> (31 - LOG2_TABBITS);

    // polynomial for log2(1+x) over x=[0,1]
    int32_t c0 = log2Table[k][0];
    int32_t c1 = log2Table[k][1];
    int32_t c2 = log2Table[k][2];

    c1 += MULHI(c0, x);
    c2 += MULHI(c1, x);

    // reconstruct result in Q26
    return (e << LOG2_FRACBITS) - (c2 >> 3);
}

//
// Peak detection and -log2(x) for float input (stereo)
// x < 2^(31-LOG2_HEADROOM) returns 0x7fffffff
// x > 2^LOG2_HEADROOM undefined
//
static inline int32_t peaklog2(float* input0, float* input1) {

    // float as integer bits
    int32_t u0 = *(int32_t*)input0;
    int32_t u1 = *(int32_t*)input1;

    // max absolute value
    u0 &= IEEE754_FABS_MASK;
    u1 &= IEEE754_FABS_MASK;
    int32_t peak = MAX(u0, u1);

    // split into e and x - 1.0
    int32_t e = IEEE754_EXPN_BIAS - (peak >> IEEE754_MANT_BITS) + LOG2_HEADROOM;
    int32_t x = (peak << (31 - IEEE754_MANT_BITS)) & 0x7fffffff;

    // saturate
    if (e > 31) {
        return 0x7fffffff;
    }

    int k = x >> (31 - LOG2_TABBITS);

    // polynomial for log2(1+x) over x=[0,1]
    int32_t c0 = log2Table[k][0];
    int32_t c1 = log2Table[k][1];
    int32_t c2 = log2Table[k][2];

    c1 += MULHI(c0, x);
    c2 += MULHI(c1, x);

    // reconstruct result in Q26
    return (e << LOG2_FRACBITS) - (c2 >> 3);
}

//
// Peak detection and -log2(x) for float input (quad)
// x < 2^(31-LOG2_HEADROOM) returns 0x7fffffff
// x > 2^LOG2_HEADROOM undefined
//
static inline int32_t peaklog2(float* input0, float* input1, float* input2, float* input3) {

    // float as integer bits
    int32_t u0 = *(int32_t*)input0;
    int32_t u1 = *(int32_t*)input1;
    int32_t u2 = *(int32_t*)input2;
    int32_t u3 = *(int32_t*)input3;

    // max absolute value
    u0 &= IEEE754_FABS_MASK;
    u1 &= IEEE754_FABS_MASK;
    u2 &= IEEE754_FABS_MASK;
    u3 &= IEEE754_FABS_MASK;
    int32_t peak = MAX(MAX(u0, u1), MAX(u2, u3));

    // split into e and x - 1.0
    int32_t e = IEEE754_EXPN_BIAS - (peak >> IEEE754_MANT_BITS) + LOG2_HEADROOM;
    int32_t x = (peak << (31 - IEEE754_MANT_BITS)) & 0x7fffffff;

    // saturate
    if (e > 31) {
        return 0x7fffffff;
    }

    int k = x >> (31 - LOG2_TABBITS);

    // polynomial for log2(1+x) over x=[0,1]
    int32_t c0 = log2Table[k][0];
    int32_t c1 = log2Table[k][1];
    int32_t c2 = log2Table[k][2];

    c1 += MULHI(c0, x);
    c2 += MULHI(c1, x);

    // reconstruct result in Q26
    return (e << LOG2_FRACBITS) - (c2 >> 3);
}

//
// Count Leading Zeros
// Emulates the CLZ (ARM) and LZCNT (x86) instruction
//
static inline int CLZ(uint32_t x) {

    if (x == 0) {
        return 32;
    }

    int e = 0;
    if (x < 0x00010000) {
        x <<= 16;
        e += 16;
    }
    if (x < 0x01000000) {
        x <<= 8;
        e += 8;
    }
    if (x < 0x10000000) {
        x <<= 4;
        e += 4;
    }
    if (x < 0x40000000) {
        x <<= 2;
        e += 2;
    }
    if (x < 0x80000000) {
        e += 1;
    }
    return e;
}

//
// Compute -log2(x) for x=[0,1] in Q31, result in Q26
// x = 0 returns 0x7fffffff
// x < 0 undefined
//
static inline int32_t fixlog2(int32_t x) {

    if (x == 0) {
        return 0x7fffffff;
    }

    // split into e and x - 1.0
    int e = CLZ((uint32_t)x);
    x <<= e;            // normalize to [0x80000000, 0xffffffff]
    x &= 0x7fffffff;    // x - 1.0

    int k = x >> (31 - LOG2_TABBITS);

    // polynomial for log2(1+x) over x=[0,1]
    int32_t c0 = log2Table[k][0];
    int32_t c1 = log2Table[k][1];
    int32_t c2 = log2Table[k][2];

    c1 += MULHI(c0, x);
    c2 += MULHI(c1, x);

    // reconstruct result in Q26
    return (e << LOG2_FRACBITS) - (c2 >> 3);
}

//
// Compute exp2(-x) for x=[0,32] in Q26, result in Q31
// x < 0 undefined
//
static inline int32_t fixexp2(int32_t x) {

    // split into e and 1.0 - x
    int e = x >> LOG2_FRACBITS;
    x = ~(x << LOG2_INTBITS) & 0x7fffffff;

    int k = x >> (31 - EXP2_TABBITS);

    // polynomial for exp2(x)
    int32_t c0 = exp2Table[k][0];
    int32_t c1 = exp2Table[k][1];
    int32_t c2 = exp2Table[k][2];

    c1 += MULHI(c0, x);
    c2 += MULHI(c1, x);

    // reconstruct result in Q31
    return c2 >> e;
}

// fast TPDF dither in [-1.0f, 1.0f]
static inline float dither() {
    static uint32_t rz = 0;
    rz = rz * 69069 + 1;
    int32_t r0 = rz & 0xffff;
    int32_t r1 = rz >> 16;
    return (int32_t)(r0 - r1) * (1/65536.0f);
}

//
// Min-hold lowpass filter
//
// Bandlimits the gain control signal to greatly reduce the modulation distortion,
// while still reaching the peak attenuation after exactly N-1 samples of delay.
// N completely determines the attack time.
// 
template<int N, int CIC1, int CIC2>
class MinFilterT {

    static_assert((N & (N - 1)) == 0, "N must be a power of 2");
    static_assert((CIC1 - 1) + (CIC2 - 1) == (N - 1), "Total CIC delay must be N-1");

    int32_t _buffer[2*N] = {};  // shared FIFO
    size_t _index = 0;

    int32_t _acc1 = 0;  // CIC1 integrator
    int32_t _acc2 = 0;  // CIC2 integrator

public:
    MinFilterT() {

        // fill history
        for (size_t n = 0; n < N-1; n++) {
            process(0x7fffffff);
        }
    }

    int32_t process(int32_t x) {

        const size_t MASK = 2*N - 1;    // buffer wrap
        size_t i = _index;

        // Fast min-hold using a running-min filter.  Finds the peak (min) value
        // in the sliding window of N-1 samples, using only log2(N) comparisons.
        // Hold time of N-1 samples exactly cancels the step response of FIR filter.

        for (size_t n = 1; n < N; n <<= 1) {

            _buffer[i] = x;
            i = (i + n) & MASK;
            x = MIN(x, _buffer[i]);
        }

        // Fast FIR attack/lowpass filter using a 2-stage CIC filter.
        // The step response reaches final value after N-1 samples.

        const int32_t CICGAIN = 0xffffffff / (CIC1 * CIC2); // Q32
        x = MULHI(x, CICGAIN);

        _buffer[i] = _acc1;
        _acc1 += x;                 // integrator
        i = (i + CIC1 - 1) & MASK;
        x = _acc1 - _buffer[i];     // comb

        _buffer[i] = _acc2;
        _acc2 += x;                 // integrator
        i = (i + CIC2 - 1) & MASK;
        x = _acc2 - _buffer[i];     // comb

        _index = (i + 1) & MASK;    // skip unused tap
        return x;
    }
};

//
// Max-hold lowpass filter
//
// Bandlimits the gain control signal to greatly reduce the modulation distortion,
// while still reaching the peak attenuation after exactly N-1 samples of delay.
// N completely determines the attack time.
// 
template<int N, int CIC1, int CIC2>
class MaxFilterT {

    static_assert((N & (N - 1)) == 0, "N must be a power of 2");
    static_assert((CIC1 - 1) + (CIC2 - 1) == (N - 1), "Total CIC delay must be N-1");

    int32_t _buffer[2*N] = {};  // shared FIFO
    size_t _index = 0;

    int32_t _acc1 = 0;  // CIC1 integrator
    int32_t _acc2 = 0;  // CIC2 integrator

public:
    MaxFilterT() {

        // fill history
        for (size_t n = 0; n < N-1; n++) {
            process(0);
        }
    }

    int32_t process(int32_t x) {

        const size_t MASK = 2*N - 1;    // buffer wrap
        size_t i = _index;

        // Fast max-hold using a running-max filter.  Finds the peak (max) value
        // in the sliding window of N-1 samples, using only log2(N) comparisons.
        // Hold time of N-1 samples exactly cancels the step response of FIR filter.

        for (size_t n = 1; n < N; n <<= 1) {

            _buffer[i] = x;
            i = (i + n) & MASK;
            x = MAX(x, _buffer[i]);
        }

        // Fast FIR attack/lowpass filter using a 2-stage CIC filter.
        // The step response reaches final value after N-1 samples.

        const int32_t CICGAIN = 0xffffffff / (CIC1 * CIC2); // Q32
        x = MULHI(x, CICGAIN);

        _buffer[i] = _acc1;
        _acc1 += x;                 // integrator
        i = (i + CIC1 - 1) & MASK;
        x = _acc1 - _buffer[i];     // comb

        _buffer[i] = _acc2;
        _acc2 += x;                 // integrator
        i = (i + CIC2 - 1) & MASK;
        x = _acc2 - _buffer[i];     // comb

        _index = (i + 1) & MASK;    // skip unused tap
        return x;
    }
};

//
// Specializations that define the optimum lowpass filter for each length.
//
template<int N> class MinFilter;
template<> class MinFilter< 16> : public MinFilterT< 16,   7,  10> {};
template<> class MinFilter< 32> : public MinFilterT< 32,  14,  19> {};
template<> class MinFilter< 64> : public MinFilterT< 64,  27,  38> {};
template<> class MinFilter<128> : public MinFilterT<128,  53,  76> {};
template<> class MinFilter<256> : public MinFilterT<256, 106, 151> {};

template<int N> class MaxFilter;
template<> class MaxFilter< 16> : public MaxFilterT< 16,   7,  10> {};
template<> class MaxFilter< 32> : public MaxFilterT< 32,  14,  19> {};
template<> class MaxFilter< 64> : public MaxFilterT< 64,  27,  38> {};
template<> class MaxFilter<128> : public MaxFilterT<128,  53,  76> {};
template<> class MaxFilter<256> : public MaxFilterT<256, 106, 151> {};

//
// N-1 sample delay (mono)
//
template<int N, typename T = float>
class MonoDelay {

    static_assert((N & (N - 1)) == 0, "N must be a power of 2");

    T _buffer[N] = {};
    size_t _index = 0;

public:
    void process(T& x) {

        const size_t MASK = N - 1;  // buffer wrap
        size_t i = _index;

        _buffer[i] = x;

        i = (i + (N - 1)) & MASK;

        x = _buffer[i];

        _index = i;
    }
};

//
// N-1 sample delay (stereo)
//
template<int N, typename T = float>
class StereoDelay {

    static_assert((N & (N - 1)) == 0, "N must be a power of 2");

    T _buffer[2*N] = {};
    size_t _index = 0;

public:
    void process(T& x0, T& x1) {

        const size_t MASK = 2*N - 1;    // buffer wrap
        size_t i = _index;

        _buffer[i+0] = x0;
        _buffer[i+1] = x1;

        i = (i + 2*(N - 1)) & MASK;

        x0 = _buffer[i+0];
        x1 = _buffer[i+1];

        _index = i;
    }
};

//
// N-1 sample delay (quad)
//
template<int N, typename T = float>
class QuadDelay {

    static_assert((N & (N - 1)) == 0, "N must be a power of 2");

    T _buffer[4*N] = {};
    size_t _index = 0;

public:
    void process(T& x0, T& x1, T& x2, T& x3) {

        const size_t MASK = 4*N - 1;    // buffer wrap
        size_t i = _index;

        _buffer[i+0] = x0;
        _buffer[i+1] = x1;
        _buffer[i+2] = x2;
        _buffer[i+3] = x3;

        i = (i + 4*(N - 1)) & MASK;

        x0 = _buffer[i+0];
        x1 = _buffer[i+1];
        x2 = _buffer[i+2];
        x3 = _buffer[i+3];

        _index = i;
    }
};
