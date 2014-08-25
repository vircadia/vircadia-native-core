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

//
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


//
// Implements common base class interface for all Audio Filter Objects
//
template< class T >
class AudioFilterBase {

protected:
    
    //
    // data
    //
    AudioBiquad _kernel;
    float _sampleRate;
    float _frequency;
    float _gain;
    float _slope;
    
    //
    // helpers
    //
    void updateKernel() {
        static_cast<T*>(this)->updateKernel();
    }
    
public:
    //
    // ctor/dtor
    //
    AudioFilterBase() {
        setParameters(0.,0.,0.,0.);
    }
    
    ~AudioFilterBase() {
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

//
// Implements a low-shelf filter using a biquad 
//
class AudioFilterLSF : 
public AudioFilterBase< AudioFilterLSF >
{
public:
    
    //
    // helpers
    //
    void updateKernel() {
        // TBD
    }
};

//
// Implements a hi-shelf filter using a biquad 
//
class AudioFilterHSF : 
public AudioFilterBase< AudioFilterHSF >
{
public:
    
    //
    // helpers
    //
    void updateKernel() {
        // TBD
    }
};

//
// Implements a single-band parametric EQ using a biquad "peaking EQ" configuration
//
class AudioFilterPEQ : 
    public AudioFilterBase< AudioFilterPEQ >
{
public:
    
    //
    // helpers
    //
    void updateKernel() {
        
        const float a       =  _gain;
        const float omega   =  TWO_PI * _frequency / _sampleRate;
        const float alpha   =  0.5f * sinf(omega) / _slope;
        
        /*
         a0 =   1 + alpha*A
         a1 =  -2*cos(w0)
         a2 =   1 - alpha*A
         b1 =  -2*cos(w0)
         b2 =   1 - alpha/A
         */
        const float a0      =  1.0f + (alpha*a);
        const float a1      = -2.0f * cosf(omega);
        const float a2      =  1.0f - (alpha*a);
        const float b1      =  a1;
        const float b2      =  1.0f - (alpha/a);
        
        const float scale   =  1.0f / ( 1.0f + (alpha/a) );
        
        _kernel.setParameters( a0*scale,a1*scale,a2*scale,b1*scale,b2*scale );
    }
};

#endif // hifi_AudioFilter_h
