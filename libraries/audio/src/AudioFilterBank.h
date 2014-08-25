//
//  AudioFilterBank.h
//  hifi
//
//  Created by Craig Hansen-Sturm on 8/23/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AudioFilterBank_h
#define hifi_AudioFilterBank_h

//
// Helper/convenience class that implements a bank of Filter objects
//
template< typename T, const int N, const int C >
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
    static const int _channelCount  = C;
    static const int _profileCount  = 4;
    
    static FilterParameter _profiles[ _profileCount ][ _filterCount ];

    //
    // private data
    //
    T           _filters[ _filterCount ][ _channelCount ];
    float*      _buffer[ _channelCount ];
    float       _sampleRate;
    uint16_t    _frameCount;

public:

    //
    // ctor/dtor
    //
    AudioFilterBank() 
    : _sampleRate(0.)
    , _frameCount(0) {
        for (int i = 0; i < _channelCount; ++i) {
            _buffer[ i ] = NULL;
        }
    }

    ~AudioFilterBank() {
        finalize();
    }

    //
    // public interface
    //
    void initialize(const float sampleRate, const int frameCount) {
        finalize();

        for (int i = 0; i < _channelCount; ++i) {
            _buffer[i] = (float*)malloc(frameCount * sizeof(float));
        }
        
        _sampleRate = sampleRate;
        _frameCount = frameCount;
        
        reset();
        loadProfile(0); // load default profile "flat response" into the bank (see AudioFilterBank.cpp)
    }

    void finalize() {
        for (int i = 0; i < _channelCount; ++i) {
            if (_buffer[i]) {
                free (_buffer[i]);
                _buffer[i] = NULL;
            }
        }
    }

    void loadProfile(int profileIndex) {
        if (profileIndex >= 0 && profileIndex < _profileCount) {
            
            for (int i = 0; i < _filterCount; ++i) {
                FilterParameter p = _profiles[profileIndex][i];

                for (int j = 0; j < _channelCount; ++j) {
                    _filters[i][j].setParameters(_sampleRate,p._p1,p._p2,p._p3);
                }
            }
        }
    }

    void setParameters(int filterStage, int filterChannel, const float sampleRate, const float frequency, const float gain, 
                       const float slope) {
        if (filterStage >= 0 && filterStage < _filterCount && filterChannel >= 0 && filterChannel < _channelCount) {
            _filters[filterStage][filterChannel].setParameters(sampleRate,frequency,gain,slope);
        }
    }

    void getParameters(int filterStage, int filterChannel, float& sampleRate, float& frequency, float& gain, float& slope) {
        if (filterStage >= 0 && filterStage < _filterCount && filterChannel >= 0 && filterChannel < _channelCount) {
            _filters[filterStage][filterChannel].getParameters(sampleRate,frequency,gain,slope);
        }
    }
    
    void render(const int16_t* in, int16_t* out, const int frameCount) {
        if (!_buffer || (frameCount > _frameCount))
            return;

        const int scale = (2 << ((8 * sizeof(int16_t)) - 1));

        // de-interleave and convert int16_t to float32 (normalized to -1. ... 1.)
        for (int i = 0; i < frameCount; ++i) {
            for (int j = 0; j < _channelCount; ++j) {
                _buffer[j][i] = ((float)(*in++)) / scale;
            }
        }

        // now step through each filter
        for (int i = 0; i < _channelCount; ++i) {
            for (int j = 0; j < _filterCount; ++j) {
                _filters[j][i].render( &_buffer[i][0], &_buffer[i][0], frameCount );
            }
        }

        // convert float32 to int16_t and interleave
        for (int i = 0; i < frameCount; ++i) {
            for (int j = 0; j < _channelCount; ++j) {
                *out++ = (int16_t)(_buffer[j][i] * scale);
            }
        }
    }

    void reset() {
        for (int i = 0; i < _filterCount; ++i) {
            for (int j = 0; j < _channelCount; ++j) {
                _filters[i][j].reset();
            }
        }
    }

};

//
// Specializations of AudioFilterBank
//
typedef AudioFilterBank< AudioFilterLSF, 1, 1> AudioFilterLSF1m; // mono bank with one band of LSF
typedef AudioFilterBank< AudioFilterLSF, 1, 2> AudioFilterLSF1s; // stereo bank with one band of LSF
typedef AudioFilterBank< AudioFilterHSF, 1, 1> AudioFilterHSF1m; // mono bank with one band of HSF
typedef AudioFilterBank< AudioFilterHSF, 1, 2> AudioFilterHSF1s; // stereo bank with one band of HSF
typedef AudioFilterBank< AudioFilterPEQ, 1, 1> AudioFilterPEQ1m; // mono bank with one band of PEQ
typedef AudioFilterBank< AudioFilterPEQ, 2, 1> AudioFilterPEQ2m; // mono bank with two bands of PEQ
typedef AudioFilterBank< AudioFilterPEQ, 3, 1> AudioFilterPEQ3m; // mono bank with three bands of PEQ
typedef AudioFilterBank< AudioFilterPEQ, 1, 2> AudioFilterPEQ1s; // stereo bank with one band of PEQ
typedef AudioFilterBank< AudioFilterPEQ, 2, 2> AudioFilterPEQ2s; // stereo bank with two bands of PEQ
typedef AudioFilterBank< AudioFilterPEQ, 3, 2> AudioFilterPEQ3s; // stereo bank with three bands of PEQ
// etc....


#endif // hifi_AudioFilter_h
