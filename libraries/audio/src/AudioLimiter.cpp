//
//  AudioLimiter.cpp
//  libraries/audio/src
//
//  Created by Ken Cooke on 2/11/15.
//  Copyright 2016 High Fidelity, Inc.
//

#include <assert.h>

#include "AudioDynamics.h"
#include "AudioLimiter.h"

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
    setThreshold(0.0f);
    setRelease(250.0f);
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

    MinFilter<N> _filter;
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

    MinFilter<N> _filter;
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

    MinFilter<N> _filter;
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
