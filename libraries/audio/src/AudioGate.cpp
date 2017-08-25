//
//  AudioGate.cpp
//  libraries/audio/src
//
//  Created by Ken Cooke on 5/5/17.
//  Copyright 2017 High Fidelity, Inc.
//

#include <string.h>
#include <assert.h>

#include "AudioDynamics.h"
#include "AudioGate.h"

// log2 domain headroom bits above 0dB (int32_t)
static const int LOG2_HEADROOM_Q30 = 1;

// convert Q30 to Q15 with saturation
static inline int32_t saturateQ30(int32_t x) {

    x = (x + (1 << 14)) >> 15;
    x = MIN(MAX(x, -32768), 32767);

    return x;
}

//
// First-order DC-blocking filter, with zero at 1.0 and pole at 0.9999
//
// -3dB @ 1.0 Hz (48KHz)
// -3dB @ 0.5 Hz (24KHz)
//
// input in Q15, output in Q30
//
class MonoDCBlock {

    int32_t _dcOffset = {};     // Q30, cannot overflow

public:
    void process(int32_t& x) {

        x <<= 15;               // scale to Q30
        x -= _dcOffset;         // remove DC
        _dcOffset += x >> 13;   // pole = (1.0 - 2^-13) = 0.9999
    }
};

class StereoDCBlock {

    int32_t _dcOffset[2] = {};

public:
    void process(int32_t& x0, int32_t& x1) {

        x0 <<= 15;
        x1 <<= 15;

        x0 -= _dcOffset[0];
        x1 -= _dcOffset[1];

        _dcOffset[0] += x0 >> 14;
        _dcOffset[1] += x1 >> 14;
    }
};

class QuadDCBlock {

    int32_t _dcOffset[4] = {};

public:
    void process(int32_t& x0, int32_t& x1, int32_t& x2, int32_t& x3) {

        x0 <<= 15;
        x1 <<= 15;
        x2 <<= 15;
        x3 <<= 15;

        x0 -= _dcOffset[0];
        x1 -= _dcOffset[1];
        x2 -= _dcOffset[2];
        x3 -= _dcOffset[3];

        _dcOffset[0] += x0 >> 14;
        _dcOffset[1] += x1 >> 14;
        _dcOffset[2] += x2 >> 14;
        _dcOffset[3] += x3 >> 14;
    }
};

//
// Gate (common)
//
class GateImpl {
protected:

    // histogram
    static const int NHIST = 256;
    int _update[NHIST] = {};
    int _histogram[NHIST] = {};

    // peakhold
    int32_t _holdMin = 0x7fffffff;
    int32_t _holdInc = 0x7fffffff;
    uint32_t _holdMax = 0x7fffffff;
    int32_t _holdRel = 0x7fffffff;
    int32_t _holdPeak = 0x7fffffff;

    // hysteresis
    int32_t _hysteresis = 0;
    int32_t _hystOffset = 0;
    int32_t _hystPeak = 0x7fffffff;

    int32_t _release = 0x7fffffff;

    int32_t _threshFixed = 0;
    int32_t _threshAdapt = 0;
    int32_t _attn = 0x7fffffff;

    int _sampleRate;

public:
    GateImpl(int sampleRate);
    virtual ~GateImpl() {}

    void setThreshold(float threshold);
    void setHold(float hold);
    void setHysteresis(float hysteresis);
    void setRelease(float release);

    void clearHistogram() { memset(_update, 0, sizeof(_update)); }
    void updateHistogram(int32_t value, int count);
    int partitionHistogram();
    void processHistogram(int numFrames);

    int32_t peakhold(int32_t peak);
    int32_t hysteresis(int32_t peak);
    int32_t envelope(int32_t attn);

    virtual void process(int16_t* input, int16_t* output, int numFrames) = 0;
    virtual void removeDC(int16_t* input, int16_t* output, int numFrames) = 0;
};

GateImpl::GateImpl(int sampleRate) {

    sampleRate = MAX(sampleRate, 8000);
    sampleRate = MIN(sampleRate, 96000);
    _sampleRate = sampleRate;

    // defaults
    setThreshold(-30.0f);
    setHold(20.0f);
    setHysteresis(6.0f);
    setRelease(1000.0f);
}

//
// Set the gate threshold (dB)
// This is a base value that is modulated by the adaptive threshold algorithm.
//
void GateImpl::setThreshold(float threshold) {

    // gate threshold = -96dB to 0dB
    threshold = MAX(threshold, -96.0f);
    threshold = MIN(threshold, 0.0f);

    // gate threshold in log2 domain
    _threshFixed = (int32_t)(-(double)threshold * DB_TO_LOG2 * (1 << LOG2_FRACBITS));
    _threshFixed += LOG2_HEADROOM_Q30 << LOG2_FRACBITS;

    _threshAdapt = _threshFixed;
}

//
// Set the detector hold time (milliseconds)
//
void GateImpl::setHold(float hold) {

    const double RELEASE = 100.0;   // release = 100ms
    const double PROGHOLD = 0.100;  // progressive hold = 100ms

    // pure hold = 1 to 1000ms
    hold = MAX(hold, 1.0f);
    hold = MIN(hold, 1000.0f);

    _holdMin = msToTc(RELEASE, _sampleRate);

    _holdInc = (int32_t)((_holdMin - 0x7fffffff) / (PROGHOLD * _sampleRate));
    _holdInc = MIN(_holdInc, -1); // prevent 0 on long releases

    _holdMax = 0x7fffffff - (uint32_t)(_holdInc * (double)hold/1000.0 * _sampleRate);
}

//
// Set the detector hysteresis (dB)
//
void GateImpl::setHysteresis(float hysteresis) {

    // gate hysteresis in log2 domain
    _hysteresis = (int32_t)((double)hysteresis * DB_TO_LOG2 * (1 << LOG2_FRACBITS));
}

//
// Set the gate release time (milliseconds)
//
void GateImpl::setRelease(float release) {

    // gate release = 50 to 5000ms
    release = MAX(release, 50.0f);
    release = MIN(release, 5000.0f);

    _release = msToTc((double)release, _sampleRate);
}

//
// Update the histogram count of the bin which contains value
//
void GateImpl::updateHistogram(int32_t value, int count = 1) {

    // quantize to LOG2 + 3 fraction bits (0.75dB steps)
    int index = (NHIST-1) - (value >> (LOG2_FRACBITS - 3));

    assert(index >= 0);
    assert(index < NHIST);

    _update[index] += count << 16;  // Q16 for filtering

    assert(_update[index] >= 0);
}

//
// Partition the histogram
//
// The idea behind the adaptive threshold:
//
// When processing a gaussian mixture of signal and noise, separated by minimal SNR, 
// a bimodal distribution emerges in the histogram of preprocessed peak levels.
// In this case, the threshold adapts toward the level that optimally partitions the distributions.
// Partitioning is computed using Otsu's method.
//
// When only a single distribution is present, the threshold becomes level-dependent:
// At levels below the fixed threshold, the threshold adapts toward the upper edge
// of the distribution, presumed to be noise.
// At levels above the fixed threshold, the threshold adapts toward the lower edge
// of the distribution, presumed to be signal.
// This is implemented by adding a hidden (bias) distribution at the fixed threshold.
// 
int GateImpl::partitionHistogram() {

    // initialize
    int total = 0;
    float sum = 0.0f;
    for (int i = 0; i < NHIST; i++) {
        total += _histogram[i];
        sum += (float)i * _histogram[i];
    }

    int w0 = 0;
    float sum0 = 0.0f;
    float max = 0.0f;
    int index = 0;

    // find the index that maximizes the between-class variance
    for (int i = 0 ; i < NHIST; i++) {

        // update weights
        w0 += _histogram[i];
        int w1 = total - w0;

        if (w0 == 0) {
            continue;   // skip leading zeros
        }
        if (w1 == 0) {
            break;      // skip trailing zeros
        }

        // update means
        sum0 += (float)i * _histogram[i];
        float sum1 = sum - sum0;

        float m0 = sum0 / (float)w0;
        float m1 = sum1 / (float)w1;

        // between-class variance
        float variance = (float)w0 * (float)w1 * (m0 - m1) * (m0 - m1);

        // update threshold
        if (variance > max) {
            max = variance;
            index = i;
        }
    }
    return index;
}

//
// Process the histogram to update the adaptive threshold
//
void GateImpl::processHistogram(int numFrames) {

    const int32_t LOG2E_Q26 = (int32_t)(log2(exp(1.0)) * (1 << LOG2_FRACBITS) + 0.5);

    // compute time constants, for sampleRate downsampled by numFrames
    int32_t tcHistogram = fixexp2(MULDIV64(numFrames, LOG2E_Q26, _sampleRate * 10));    // 10 seconds
    int32_t tcThreshold = fixexp2(MULDIV64(numFrames, LOG2E_Q26, _sampleRate * 1));     // 1 second
    
    // add bias at the fixed threshold
    updateHistogram(_threshFixed, (numFrames+7)/8);

    // leaky integrate into long-term histogram
    for (int i = 0; i < NHIST; i++) {
        _histogram[i] = _update[i] + MULQ31((_histogram[i] - _update[i]), tcHistogram);
    }

    // compute new threshold
    int index = partitionHistogram();
    int32_t threshold = ((NHIST-1) - index) << (LOG2_FRACBITS - 3);

    // smooth threshold update
    _threshAdapt = threshold + MULQ31((_threshAdapt - threshold), tcThreshold);

    //printf("threshold = %0.1f\n", (_threshAdapt - (LOG2_HEADROOM_Q15 << LOG2_FRACBITS)) * -6.02f / (1 << LOG2_FRACBITS));
}

//
// Gate detector peakhold
//
int32_t GateImpl::peakhold(int32_t peak) {

    if (peak > _holdPeak) {

        // RELEASE
        // 3-stage progressive hold
        //
        // (_holdRel > 0x7fffffff) pure hold
        // (_holdRel > _holdMin) progressive hold
        // (_holdRel = _holdMin) release

        _holdRel += _holdInc;                                   // update progressive hold
        _holdRel = MAX((uint32_t)_holdRel, (uint32_t)_holdMin); // saturate at final value

        int32_t tc = MIN((uint32_t)_holdRel, 0x7fffffff);
        peak += MULQ31((_holdPeak - peak), tc);                 // apply release

    } else {

        // ATTACK
        _holdRel = _holdMax;    // reset release
    }
    _holdPeak = peak;

    return peak;
}

//
// Gate hysteresis
// Implemented as detector hysteresis instead of high/low thresholds, to simplify adaptive threshold.
//
int32_t GateImpl::hysteresis(int32_t peak) {

    // by construction, cannot overflow/underflow
    assert((double)_hystOffset + (peak - _hystPeak) <= +(double)0x7fffffff);
    assert((double)_hystOffset + (peak - _hystPeak) >= -(double)0x80000000);

    // accumulate the offset, with saturation
    _hystOffset += peak - _hystPeak;
    _hystOffset = MIN(MAX(_hystOffset, 0), _hysteresis);

    _hystPeak = peak;
    peak -= _hystOffset;    // apply hysteresis

    assert(peak >= 0);
    return peak;
}

//
// Gate envelope processing
// zero attack, fixed release
//
int32_t GateImpl::envelope(int32_t attn) {

    if (attn > _attn) {
        attn += MULQ31((_attn - attn), _release);   // apply release
    }
    _attn = attn;

    return attn;
}

//
// Gate (mono)
//
template<int N>
class GateMono : public GateImpl {

    MonoDCBlock _dc;
    MaxFilter<N> _filter;
    MonoDelay<N, int32_t> _delay;

public:
    GateMono(int sampleRate) : GateImpl(sampleRate) {}

    // mono input/output (in-place is allowed)
    void process(int16_t* input, int16_t* output, int numFrames) override;
    void removeDC(int16_t* input, int16_t* output, int numFrames) override;
};

template<int N>
void GateMono<N>::process(int16_t* input, int16_t* output, int numFrames) {

    clearHistogram();

    for (int n = 0; n < numFrames; n++) {

        int32_t x = input[n];

        // remove DC
        _dc.process(x);

        // peak detect
        int32_t peak = abs(x);

        // convert to log2 domain
        peak = fixlog2(peak);

        // apply peak hold
        peak = peakhold(peak);

        // count peak level
        updateHistogram(peak);

        // apply hysteresis
        peak = hysteresis(peak);

        // compute gate attenuation
        int32_t attn = (peak > _threshAdapt) ? 0x7fffffff : 0;    // hard-knee, 1:inf ratio

        // apply envelope
        attn = envelope(attn);

        // convert from log2 domain
        attn = fixexp2(attn);

        // lowpass filter
        attn = _filter.process(attn);

        // delay audio
        _delay.process(x);

        // apply gain
        x = MULQ31(x, attn);

        // store 16-bit output
        output[n] = (int16_t)saturateQ30(x);
    }

    // update adaptive threshold
    processHistogram(numFrames);
}

template<int N>
void GateMono<N>::removeDC(int16_t* input, int16_t* output, int numFrames) {

    for (int n = 0; n < numFrames; n++) {

        int32_t x = input[n];

        // remove DC
        _dc.process(x);

        // store 16-bit output
        output[n] = (int16_t)saturateQ30(x);
    }
}

//
// Gate (stereo)
//
template<int N>
class GateStereo : public GateImpl {

    StereoDCBlock _dc;
    MaxFilter<N> _filter;
    StereoDelay<N, int32_t> _delay;

public:
    GateStereo(int sampleRate) : GateImpl(sampleRate) {}

    // interleaved stereo input/output (in-place is allowed)
    void process(int16_t* input, int16_t* output, int numFrames) override;
    void removeDC(int16_t* input, int16_t* output, int numFrames) override;
};

template<int N>
void GateStereo<N>::process(int16_t* input, int16_t* output, int numFrames) {

    clearHistogram();

    for (int n = 0; n < numFrames; n++) {

        int32_t x0 = input[2*n+0];
        int32_t x1 = input[2*n+1];

        // remove DC
        _dc.process(x0, x1);

        // peak detect
        int32_t peak = MAX(abs(x0), abs(x1));

        // convert to log2 domain
        peak = fixlog2(peak);

        // apply peak hold
        peak = peakhold(peak);

        // count peak level
        updateHistogram(peak);

        // apply hysteresis
        peak = hysteresis(peak);

        // compute gate attenuation
        int32_t attn = (peak > _threshAdapt) ? 0x7fffffff : 0;    // hard-knee, 1:inf ratio

        // apply envelope
        attn = envelope(attn);

        // convert from log2 domain
        attn = fixexp2(attn);

        // lowpass filter
        attn = _filter.process(attn);

        // delay audio
        _delay.process(x0, x1);

        // apply gain
        x0 = MULQ31(x0, attn);
        x1 = MULQ31(x1, attn);

        // store 16-bit output
        output[2*n+0] = (int16_t)saturateQ30(x0);
        output[2*n+1] = (int16_t)saturateQ30(x1);
    }

    // update adaptive threshold
    processHistogram(numFrames);
}

template<int N>
void GateStereo<N>::removeDC(int16_t* input, int16_t* output, int numFrames) {

    for (int n = 0; n < numFrames; n++) {

        int32_t x0 = input[2*n+0];
        int32_t x1 = input[2*n+1];

        // remove DC
        _dc.process(x0, x1);

        // store 16-bit output
        output[2*n+0] = (int16_t)saturateQ30(x0);
        output[2*n+1] = (int16_t)saturateQ30(x1);
    }
}

//
// Gate (quad)
//
template<int N>
class GateQuad : public GateImpl {

    QuadDCBlock _dc;
    MaxFilter<N> _filter;
    QuadDelay<N, int32_t> _delay;

public:
    GateQuad(int sampleRate) : GateImpl(sampleRate) {}

    // interleaved quad input/output (in-place is allowed)
    void process(int16_t* input, int16_t* output, int numFrames) override;
    void removeDC(int16_t* input, int16_t* output, int numFrames) override;
};

template<int N>
void GateQuad<N>::process(int16_t* input, int16_t* output, int numFrames) {

    clearHistogram();

    for (int n = 0; n < numFrames; n++) {

        int32_t x0 = input[4*n+0];
        int32_t x1 = input[4*n+1];
        int32_t x2 = input[4*n+2];
        int32_t x3 = input[4*n+3];

        // remove DC
        _dc.process(x0, x1, x2, x3);

        // peak detect
        int32_t peak = MAX(MAX(abs(x0), abs(x1)), MAX(abs(x2), abs(x3)));

        // convert to log2 domain
        peak = fixlog2(peak);

        // apply peak hold
        peak = peakhold(peak);

        // count peak level
        updateHistogram(peak);

        // apply hysteresis
        peak = hysteresis(peak);

        // compute gate attenuation
        int32_t attn = (peak > _threshAdapt) ? 0x7fffffff : 0;    // hard-knee, 1:inf ratio

        // apply envelope
        attn = envelope(attn);

        // convert from log2 domain
        attn = fixexp2(attn);

        // lowpass filter
        attn = _filter.process(attn);

        // delay audio
        _delay.process(x0, x1, x2, x3);

        // apply gain
        x0 = MULQ31(x0, attn);
        x1 = MULQ31(x1, attn);
        x2 = MULQ31(x2, attn);
        x3 = MULQ31(x3, attn);

        // store 16-bit output
        output[4*n+0] = (int16_t)saturateQ30(x0);
        output[4*n+1] = (int16_t)saturateQ30(x1);
        output[4*n+2] = (int16_t)saturateQ30(x2);
        output[4*n+3] = (int16_t)saturateQ30(x3);
    }

    // update adaptive threshold
    processHistogram(numFrames);
}

template<int N>
void GateQuad<N>::removeDC(int16_t* input, int16_t* output, int numFrames) {

    for (int n = 0; n < numFrames; n++) {

        int32_t x0 = input[4*n+0];
        int32_t x1 = input[4*n+1];
        int32_t x2 = input[4*n+2];
        int32_t x3 = input[4*n+3];

        // remove DC
        _dc.process(x0, x1, x2, x3);

        // store 16-bit output
        output[4*n+0] = (int16_t)saturateQ30(x0);
        output[4*n+1] = (int16_t)saturateQ30(x1);
        output[4*n+2] = (int16_t)saturateQ30(x2);
        output[4*n+3] = (int16_t)saturateQ30(x3);
    }
}

//
// Public API
//

AudioGate::AudioGate(int sampleRate, int numChannels) {

    if (numChannels == 1) {

        // ~3ms lookahead for all rates
        if (sampleRate < 16000) {
            _impl = new GateMono<32>(sampleRate);
        } else if (sampleRate < 32000) {
            _impl = new GateMono<64>(sampleRate);
        } else if (sampleRate < 64000) {
            _impl = new GateMono<128>(sampleRate);
        } else {
            _impl = new GateMono<256>(sampleRate);
        }

    } else if (numChannels == 2) {

        // ~3ms lookahead for all rates
        if (sampleRate < 16000) {
            _impl = new GateStereo<32>(sampleRate);
        } else if (sampleRate < 32000) {
            _impl = new GateStereo<64>(sampleRate);
        } else if (sampleRate < 64000) {
            _impl = new GateStereo<128>(sampleRate);
        } else {
            _impl = new GateStereo<256>(sampleRate);
        }

    } else if (numChannels == 4) {

        // ~3ms lookahead for all rates
        if (sampleRate < 16000) {
            _impl = new GateQuad<32>(sampleRate);
        } else if (sampleRate < 32000) {
            _impl = new GateQuad<64>(sampleRate);
        } else if (sampleRate < 64000) {
            _impl = new GateQuad<128>(sampleRate);
        } else {
            _impl = new GateQuad<256>(sampleRate);
        }

    } else {
        assert(0); // unsupported
    }
}

AudioGate::~AudioGate() {
    delete _impl;
}

void AudioGate::render(int16_t* input, int16_t* output, int numFrames) {
    _impl->process(input, output, numFrames);
}

void AudioGate::removeDC(int16_t* input, int16_t* output, int numFrames) {
    _impl->removeDC(input, output, numFrames);
}

void AudioGate::setThreshold(float threshold) {
    _impl->setThreshold(threshold);
}

void AudioGate::setRelease(float release) {
    _impl->setRelease(release);
}
