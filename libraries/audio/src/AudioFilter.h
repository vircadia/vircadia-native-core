//
//  AudioFilter.h
//  hifi
//
//  Created by Craig Hansen-Sturm on 8/9/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AudioFilter_h
#define hifi_AudioFilter_h

////////////////////////////////////////////////////////////////////////////////////////////
// Implements a standard biquad filter in "Direct Form 1"
// Reference http://www.musicdsp.org/files/Audio-EQ-Cookbook.txt
//
class AudioBiquad {

    //
    // private data
    //
    float _a0;  // gain
    float _a1;  // feedforward 1
    float _a2;  // feedforward 2
    float _b1;  // feedback 1
    float _b2;  // feedback 2

    float _xm1;
    float _xm2;
    float _ym1;
    float _ym2;

public:

    //
    // ctor/dtor
    //
    AudioBiquad() 
    : _xm1(0.)
    , _xm2(0.)
    , _ym1(0.)
    , _ym2(0.) {
        setParameters(0.,0.,0.,0.,0.);
    }

    ~AudioBiquad() {
    }

    //
    // public interface
    //
    void setParameters( const float a0, const float a1, const float a2, const float b1, const float b2 ) {
        _a0 = a0; _a1 = a1; _a2 = a2; _b1 = b1; _b2 = b2;
    }

    void getParameters( float& a0, float& a1, float& a2, float& b1, float& b2 ) {
        a0 = _a0; a1 = _a1; a2 = _a2; b1 = _b1; b2 = _b2;
    }

    void render( const float* in, float* out, const int frames) {
        
        float x;
        float y;

        for (int i = 0; i < frames; ++i) {

            x = *in++;

            // biquad
            y = (_a0 * x)
              + (_a1 * _xm1) 
              + (_a2 * _xm2)
              - (_b1 * _ym1) 
              - (_b2 * _ym2);

            // update delay line
            _xm2 = _xm1;
            _xm1 = x;
            _ym2 = _ym1;
            _ym1 = y;

            *out++ = y;
        }
    }

    void reset() {
        _xm1 = _xm2 = _ym1 = _ym2 = 0.;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////
// Implements a single-band parametric EQ using a biquad "peaking EQ" configuration
//
//   gain > 1.0 boosts the center frequency
//   gain < 1.0 cuts the center frequency
//
class AudioParametricEQ {

    //
    // private data
    //
    AudioBiquad _kernel;
    float _sampleRate;
    float _frequency;
    float _gain;
    float _slope;

    // helpers
    void updateKernel() {

        /*
         a0 =   1 + alpha*A
         a1 =  -2*cos(w0)
         a2 =   1 - alpha*A
         b1 =  -2*cos(w0)
         b2 =   1 - alpha/A
       */

        const float a       =  _gain;
        const float omega   =  TWO_PI * _frequency / _sampleRate;
        const float alpha   =  0.5f * sinf(omega) / _slope;
        const float gamma   =  1.0f / ( 1.0f + (alpha/a) );

        const float a0      =  1.0f + (alpha*a);
        const float a1      = -2.0f * cosf(omega);
        const float a2      =  1.0f - (alpha*a);
        const float b1      =  a1;
        const float b2      =  1.0f - (alpha/a);

        _kernel.setParameters( a0*gamma,a1*gamma,a2*gamma,b1*gamma,b2*gamma );
    }

public:
    //
    // ctor/dtor
    //
    AudioParametricEQ() {

        setParameters(0.,0.,0.,0.);
        updateKernel();
    }

    ~AudioParametricEQ() {
    }

    //
    // public interface
    //
    void setParameters( const float sampleRate, const float frequency, const float gain, const float slope ) {

        _sampleRate = std::max(sampleRate,1.0f);
        _frequency  = std::max(frequency,2.0f);
        _gain       = std::max(gain,0.0f);
        _slope      = std::max(slope,0.00001f);

        updateKernel();
    }

    void getParameters( float& sampleRate, float& frequency, float& gain, float& slope ) {
        sampleRate = _sampleRate; frequency = _frequency; gain = _gain; slope = _slope;
    }

    void render(const float* in, float* out, const int frames ) {
        _kernel.render(in,out,frames);
    }

    void reset() {
        _kernel.reset();
    }
};

////////////////////////////////////////////////////////////////////////////////////////////
// Helper/convenience class that implements a bank of EQ objects
//
template< typename T, const int N>
class AudioFilterBank {

    //
    // types
    //
    struct FilterParameter {
        float _p1;
        float _p2;
        float _p3;
    };

    //
    // private static data
    //
    static const int _filterCount   = N;
    static const int _profileCount  = 4;

    static FilterParameter _profiles[_profileCount][_filterCount];

    //
    // private data
    //
    T           _filters[ _filterCount ];
    float*      _buffer;
    float       _sampleRate;
    uint16_t    _frameCount;

public:

    //
    // ctor/dtor
    //
    AudioFilterBank() 
    : _buffer(NULL)
    , _sampleRate(0.)
    , _frameCount(0) {
    }

    ~AudioFilterBank() {
        finalize();
    }

    //
    // public interface
    //
    void initialize( const float sampleRate, const int frameCount ) {
        finalize();
        
        _buffer = (float*)malloc( frameCount * sizeof(float) );
        if(!_buffer) {
            return;
        }
        
        _sampleRate = sampleRate;
        _frameCount = frameCount;
        
        reset();
        loadProfile(0); // load default profile "flat response" into the bank (see AudioFilter.cpp)
    }

    void finalize() {
        if (_buffer ) {
            free (_buffer);
            _buffer = NULL;
        }
    }

    void loadProfile( int profileIndex ) {
        if (profileIndex >= 0 && profileIndex < _profileCount) {
            
            for (int i = 0; i < _filterCount; ++i) {
                FilterParameter p = _profiles[profileIndex][i];

                _filters[i].setParameters(_sampleRate,p._p1,p._p2,p._p3);
            }
        }
    }

    void render( const float* in, float* out, const int frameCount ) {
        for (int i = 0; i < _filterCount; ++i) {
            _filters[i].render( in, out, frameCount );
        }
    }

    void render( const int16_t* in, int16_t* out, const int frameCount ) {
        if (!_buffer || ( frameCount > _frameCount ))
            return;

        const int scale = (2 << ((8*sizeof(int16_t))-1));

        // convert int16_t to float32 (normalized to -1. ... 1.)
        for (int i = 0; i < frameCount; ++i) {
            _buffer[i] = ((float)(*in++)) / scale;
        }
        // for this filter, we share input/output buffers at each stage, but our design does not mandate this
        render( _buffer, _buffer, frameCount );

        // convert float32 to int16_t
        for (int i = 0; i < frameCount; ++i) {
           *out++ = (int16_t)(_buffer[i] * scale);
        }
    }

    void reset() {
        for (int i = 0; i < _filterCount; ++i ) {
            _filters[i].reset();
        }
    }

};

////////////////////////////////////////////////////////////////////////////////////////////
// Specializations of AudioFilterBank
//
typedef AudioFilterBank< AudioParametricEQ, 1> AudioFilterPEQ1; // bank with one band of PEQ
typedef AudioFilterBank< AudioParametricEQ, 2> AudioFilterPEQ2; // bank with two bands of PEQ
typedef AudioFilterBank< AudioParametricEQ, 3> AudioFilterPEQ3; // bank with three bands of PEQ
// etc....


#endif // hifi_AudioFilter_h
