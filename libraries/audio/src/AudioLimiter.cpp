//
//  AudioLimiter.cpp
//  libraries/audio/src
//
//  Created by Ken Cooke on 2/11/15.
//  Copyright 2016 High Fidelity, Inc.
//

#include <math.h>
#include <assert.h>

#include "AudioLimiter.h"

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif 
#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifdef _MSC_VER

#include <intrin.h>
#define MUL64(a,b)  __emul((a), (b))
#define MULHI(a,b)  ((int)(MUL64(a, b) >> 32))
#define MULQ31(a,b) ((int)(MUL64(a, b) >> 31))

#else

#define MUL64(a,b)  ((long long)(a) * (b))
#define MULHI(a,b)  ((int)(MUL64(a, b) >> 32))
#define MULQ31(a,b) ((int)(MUL64(a, b) >> 31))

#endif  // _MSC_VER

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
static double dBToGain(double dB) {
    return pow(10.0, dB / 20.0);
}

// convert milliseconds to first-order time constant
static int32_t msToTc(double ms, double sampleRate) {
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
// Compute exp2(-x) for x=[0,32] in Q26, result in Q31
// x < 0 undefined
//
static inline int32_t fixexp2(int32_t x) {

    // split into e and 1.0 - x
    int32_t e = x >> LOG2_FRACBITS;
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
// Peak-hold lowpass filter
//
// Bandlimits the gain control signal to greatly reduce the modulation distortion,
// while still reaching the peak attenuation after exactly N-1 samples of delay.
// N completely determines the limiter attack time.
// 
template<int N, int CIC1, int CIC2>
class PeakFilterT {

    static_assert((N & (N - 1)) == 0, "N must be a power of 2");
    static_assert((CIC1 - 1) + (CIC2 - 1) == (N - 1), "Total CIC delay must be N-1");

    int32_t _buffer[2*N] = {};  // shared FIFO
    size_t _index = 0;

    int32_t _acc1 = 0;  // CIC1 integrator
    int32_t _acc2 = 0;  // CIC2 integrator

public:
    PeakFilterT() {

        // fill history
        for (size_t n = 0; n < N-1; n++) {
            process(0x7fffffff);
        }
    }

    int32_t process(int32_t x) {

        const size_t MASK = 2*N - 1;    // buffer wrap
        size_t i = _index;

        // Fast peak-hold using a running-min filter.  Finds the peak (min) value
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
// Specializations that define the optimum lowpass filter for each length.
//
template<int N> class PeakFilter;

template<> class PeakFilter< 16> : public PeakFilterT< 16,   7,  10> {};
template<> class PeakFilter< 32> : public PeakFilterT< 32,  14,  19> {};
template<> class PeakFilter< 64> : public PeakFilterT< 64,  27,  38> {};
template<> class PeakFilter<128> : public PeakFilterT<128,  53,  76> {};
template<> class PeakFilter<256> : public PeakFilterT<256, 106, 151> {};

//
// N-1 sample delay (mono)
//
template<int N>
class MonoDelay {

    static_assert((N & (N - 1)) == 0, "N must be a power of 2");

    float _buffer[N] = {};
    size_t _index = 0;

public:
    void process(float& x) {

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
template<int N>
class StereoDelay {

    static_assert((N & (N - 1)) == 0, "N must be a power of 2");

    float _buffer[2*N] = {};
    size_t _index = 0;

public:
    void process(float& x0, float& x1) {

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
template<int N>
class QuadDelay {

    static_assert((N & (N - 1)) == 0, "N must be a power of 2");

    float _buffer[4*N] = {};
    size_t _index = 0;

public:
    void process(float& x0, float& x1, float& x2, float& x3) {

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

//
// Limiter (common)
//
class LimiterImpl {
protected:

    static const int NARC = 64;
    int32_t _holdTable[NARC];
    int32_t _releaseTable[NARC];

    int32_t _rmsAttack = 0x7fffffff;
    int32_t _rmsRelease = 0x7fffffff;
    int32_t _arcRelease = 0x7fffffff;

    int32_t _threshold = 0;
    int32_t _attn = 0;
    int32_t _rms = 0;
    int32_t _arc = 0;

    int _sampleRate;
    float _outGain = 0.0f;

public:
    LimiterImpl(int sampleRate);
    virtual ~LimiterImpl() {}

    void setThreshold(float threshold);
    void setRelease(float release);

    int32_t envelope(int32_t attn);

    virtual void process(float* input, int16_t* output, int numFrames) = 0;
};

LimiterImpl::LimiterImpl(int sampleRate) {

    sampleRate = MAX(sampleRate, 8000);
    sampleRate = MIN(sampleRate, 96000);
    _sampleRate = sampleRate;

    // defaults
    setThreshold(0.0);
    setRelease(250.0);
}

//
// Set the limiter threshold (dB)
// Brickwall limiting will begin when the signal exceeds the threshold.
// Makeup gain is applied, to reach but never exceed the output ceiling.
//
void LimiterImpl::setThreshold(float threshold) {

    const double OUT_CEILING = -0.3;    // cannot be 0.0, due to dither
    const double Q31_TO_Q15 = 32768 / 2147483648.0;

    // limiter threshold = -48dB to 0dB
    threshold = MAX(threshold, -48.0f);
    threshold = MIN(threshold, 0.0f);

    // limiter threshold in log2 domain
    _threshold = (int32_t)(-(double)threshold * DB_TO_LOG2 * (1 << LOG2_FRACBITS));
    _threshold += LOG2_BIAS + EXP2_BIAS;
    _threshold += LOG2_HEADROOM << LOG2_FRACBITS;

    // makeup gain and conversion to 16-bit
    _outGain = (float)(dBToGain(OUT_CEILING - (double)threshold) * Q31_TO_Q15);
}

//
// Set the limiter release time (milliseconds)
// This is a base value that scales the adaptive hold and release algorithms.
//
void LimiterImpl::setRelease(float release) {

    const double MAXHOLD = 0.100;   // max hold = 100ms
    const double MINREL = 0.025;    // min release = 0.025 * release
    const int NHOLD = 16;           // adaptive hold to adaptive release transition

    // limiter release = 50 to 5000ms
    release = MAX(release, 50.0f);
    release = MIN(release, 5000.0f);

    int32_t maxRelease = msToTc((double)release, _sampleRate);

    _rmsAttack = msToTc(0.1 * (double)release, _sampleRate);
    _rmsRelease = maxRelease;

    // Compute ARC tables, working from low peak/rms to high peak/rms.
    //
    // At low peak/rms, release = max and hold is progressive to max
    // At high peak/rms, hold = 0 and release is progressive to min

    double x = MAXHOLD * _sampleRate;
    double xstep = x / NHOLD;   // 1.0 to 1.0/NHOLD

    int i = 0;
    for (; i < NHOLD; i++) {

        // max release
        _releaseTable[i] = maxRelease;

        // progressive hold
        _holdTable[i] = (int32_t)((maxRelease - 0x7fffffff) / x);
        _holdTable[i] = MIN(_holdTable[i], -1); // prevent 0 on long releases

        x -= xstep;
        x = MAX(x, 1.0);
    }

    x = release;
    xstep = x * (1.0-MINREL) / (NARC-NHOLD-1);  // 1.0 to MINREL

    for (; i < NARC; i++) {

        // progressive release
        _releaseTable[i] = msToTc(x, _sampleRate);

        // min hold
        _holdTable[i] = (_releaseTable[i] - 0x7fffffff);    // 1 sample

        x -= xstep;
    }
}

//
// Limiter envelope processing
// zero attack, adaptive hold and release
//
int32_t LimiterImpl::envelope(int32_t attn) {

    // table of (1/attn) for 1dB to 6dB, rounded to prevent overflow
    static const int16_t invTable[64] = {
        0x6000, 0x6000, 0x6000, 0x6000, 0x6000, 0x6000, 0x6000, 0x6000, 
        0x6000, 0x6000, 0x5d17, 0x5555, 0x4ec4, 0x4924, 0x4444, 0x4000, 
        0x3c3c, 0x38e3, 0x35e5, 0x3333, 0x30c3, 0x2e8b, 0x2c85, 0x2aaa, 
        0x28f5, 0x2762, 0x25ed, 0x2492, 0x234f, 0x2222, 0x2108, 0x2000, 
        0x1f07, 0x1e1e, 0x1d41, 0x1c71, 0x1bac, 0x1af2, 0x1a41, 0x1999, 
        0x18f9, 0x1861, 0x17d0, 0x1745, 0x16c1, 0x1642, 0x15c9, 0x1555, 
        0x14e5, 0x147a, 0x1414, 0x13b1, 0x1352, 0x12f6, 0x129e, 0x1249, 
        0x11f7, 0x11a7, 0x115b, 0x1111, 0x10c9, 0x1084, 0x1041, 0x1000, 
    };

    if (attn < _attn) {

        // RELEASE
        // update release before use, to implement hold = 0

        _arcRelease += _holdTable[_arc];                        // update progressive hold
        _arcRelease = MAX(_arcRelease, _releaseTable[_arc]);    // saturate at final value

        attn += MULQ31((_attn - attn), _arcRelease);            // apply release

    } else {

        // ATTACK
        // update ARC with normalized peak/rms
        //
        // arc = (attn-rms)*6/1    for attn < 1dB
        // arc = (attn-rms)*6/attn for attn = 1dB to 6dB
        // arc = (attn-rms)*6/6    for attn > 6dB

        size_t bits = MIN(attn >> 20, 0x3f);    // saturate 1/attn at 6dB
        _arc = MAX(attn - _rms, 0);             // peak/rms = (attn-rms)
        _arc = MULHI(_arc, invTable[bits]);     // normalized peak/rms = (attn-rms)/attn
        _arc = MIN(_arc, NARC - 1);             // saturate at 6dB

        _arcRelease = 0x7fffffff;               // reset release
    }
    _attn = attn;

    // Update the RMS estimate after release is applied.
    // The feedback loop with adaptive hold will damp any sustained modulation distortion.

    int32_t tc = (attn > _rms) ? _rmsAttack : _rmsRelease;
    _rms = attn + MULQ31((_rms - attn), tc);

    return attn;
}

//
// Limiter (mono)
//
template<int N>
class LimiterMono : public LimiterImpl {

    PeakFilter<N> _filter;
    MonoDelay<N> _delay;

public:
    LimiterMono(int sampleRate) : LimiterImpl(sampleRate) {}

    void process(float* input, int16_t* output, int numFrames) override;
};

template<int N>
void LimiterMono<N>::process(float* input, int16_t* output, int numFrames) {

    for (int n = 0; n < numFrames; n++) {

        // peak detect and convert to log2 domain
        int32_t peak = peaklog2(&input[n]);

        // compute limiter attenuation
        int32_t attn = MAX(_threshold - peak, 0);

        // apply envelope
        attn = envelope(attn);

        // convert from log2 domain
        attn = fixexp2(attn);

        // lowpass filter
        attn = _filter.process(attn);
        float gain = attn * _outGain;

        // delay audio
        float x = input[n];
        _delay.process(x);

        // apply gain
        x *= gain;

        // apply dither
        x += dither();

        // store 16-bit output
        output[n] = (int16_t)floatToInt(x);
    }
}

//
// Limiter (stereo)
//
template<int N>
class LimiterStereo : public LimiterImpl {

    PeakFilter<N> _filter;
    StereoDelay<N> _delay;

public:
    LimiterStereo(int sampleRate) : LimiterImpl(sampleRate) {}

    // interleaved stereo input/output
    void process(float* input, int16_t* output, int numFrames) override;
};

template<int N>
void LimiterStereo<N>::process(float* input, int16_t* output, int numFrames) {

    for (int n = 0; n < numFrames; n++) {

        // peak detect and convert to log2 domain
        int32_t peak = peaklog2(&input[2*n+0], &input[2*n+1]);

        // compute limiter attenuation
        int32_t attn = MAX(_threshold - peak, 0);

        // apply envelope
        attn = envelope(attn);

        // convert from log2 domain
        attn = fixexp2(attn);

        // lowpass filter
        attn = _filter.process(attn);
        float gain = attn * _outGain;

        // delay audio
        float x0 = input[2*n+0];
        float x1 = input[2*n+1];
        _delay.process(x0, x1);

        // apply gain
        x0 *= gain;
        x1 *= gain;

        // apply dither
        float d = dither();
        x0 += d;
        x1 += d;

        // store 16-bit output
        output[2*n+0] = (int16_t)floatToInt(x0);
        output[2*n+1] = (int16_t)floatToInt(x1);
    }
}

//
// Limiter (quad)
//
template<int N>
class LimiterQuad : public LimiterImpl {

    PeakFilter<N> _filter;
    QuadDelay<N> _delay;

public:
    LimiterQuad(int sampleRate) : LimiterImpl(sampleRate) {}

    // interleaved quad input/output
    void process(float* input, int16_t* output, int numFrames) override;
};

template<int N>
void LimiterQuad<N>::process(float* input, int16_t* output, int numFrames) {

    for (int n = 0; n < numFrames; n++) {

        // peak detect and convert to log2 domain
        int32_t peak = peaklog2(&input[4*n+0], &input[4*n+1], &input[4*n+2], &input[4*n+3]);

        // compute limiter attenuation
        int32_t attn = MAX(_threshold - peak, 0);

        // apply envelope
        attn = envelope(attn);

        // convert from log2 domain
        attn = fixexp2(attn);

        // lowpass filter
        attn = _filter.process(attn);
        float gain = attn * _outGain;

        // delay audio
        float x0 = input[4*n+0];
        float x1 = input[4*n+1];
        float x2 = input[4*n+2];
        float x3 = input[4*n+3];
        _delay.process(x0, x1, x2, x3);

        // apply gain
        x0 *= gain;
        x1 *= gain;
        x2 *= gain;
        x3 *= gain;

        // apply dither
        float d = dither();
        x0 += d;
        x1 += d;
        x2 += d;
        x3 += d;

        // store 16-bit output
        output[4*n+0] = (int16_t)floatToInt(x0);
        output[4*n+1] = (int16_t)floatToInt(x1);
        output[4*n+2] = (int16_t)floatToInt(x2);
        output[4*n+3] = (int16_t)floatToInt(x3);
    }
}

//
// Public API
//

AudioLimiter::AudioLimiter(int sampleRate, int numChannels) {

    if (numChannels == 1) {

        // ~1.5ms lookahead for all rates
        if (sampleRate < 16000) {
            _impl = new LimiterMono<16>(sampleRate);
        } else if (sampleRate < 32000) {
            _impl = new LimiterMono<32>(sampleRate);
        } else if (sampleRate < 64000) {
            _impl = new LimiterMono<64>(sampleRate);
        } else {
            _impl = new LimiterMono<128>(sampleRate);
        }

    } else if (numChannels == 2) {

        // ~1.5ms lookahead for all rates
        if (sampleRate < 16000) {
            _impl = new LimiterStereo<16>(sampleRate);
        } else if (sampleRate < 32000) {
            _impl = new LimiterStereo<32>(sampleRate);
        } else if (sampleRate < 64000) {
            _impl = new LimiterStereo<64>(sampleRate);
        } else {
            _impl = new LimiterStereo<128>(sampleRate);
        }

    } else if (numChannels == 4) {

        // ~1.5ms lookahead for all rates
        if (sampleRate < 16000) {
            _impl = new LimiterQuad<16>(sampleRate);
        } else if (sampleRate < 32000) {
            _impl = new LimiterQuad<32>(sampleRate);
        } else if (sampleRate < 64000) {
            _impl = new LimiterQuad<64>(sampleRate);
        } else {
            _impl = new LimiterQuad<128>(sampleRate);
        }

    } else {
        assert(0); // unsupported
    }
}

AudioLimiter::~AudioLimiter() {
    delete _impl;
}

void AudioLimiter::render(float* input, int16_t* output, int numFrames) {
    _impl->process(input, output, numFrames);
}

void AudioLimiter::setThreshold(float threshold) {
    _impl->setThreshold(threshold);
}

void AudioLimiter::setRelease(float release) {
    _impl->setRelease(release);
}
