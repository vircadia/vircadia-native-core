//
//  AudioSourceTone.h
//  hifi
//
//  Created by Craig Hansen-Sturm on 9/1/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AudioSourceTone_h
#define hifi_AudioSourceTone_h

// Implements a two-pole Gordon-Smith oscillator
class AudioSourceTone
{
    float32_t _frequency;
    float32_t _amplitude;
    float32_t _sampleRate;
    float32_t _omega;
    float32_t _epsilon;
    float32_t _yq1;  
    float32_t _y1;
    
    void updateCoefficients() {
        _omega = _frequency / _sampleRate * TWO_PI;
        _epsilon = 2.0f * sinf(_omega / 2.0f);
        _yq1 = cosf(-1.0f * _omega);
        _y1 = sinf(+1.04 * _omega);
    }
    
public:
    AudioSourceTone() {
        initialize();
    }
    
    ~AudioSourceTone() {
        finalize();
    }
    
    void initialize() {
        setParameters(SAMPLE_RATE, 220.0f, 0.708f);
    }
    
    void finalize() {
    }
    
    void reset() {
    }

    void setParameters(const float32_t sampleRate, const float32_t frequency,  const float32_t amplitude) {
        _sampleRate = std::max(sampleRate, 1.0f);
        _frequency = std::max(frequency, 1.0f);
        _amplitude = std::max(amplitude, 1.0f);
        updateCoefficients();
    }
    
    void getParameters(float32_t& sampleRate, float32_t& frequency, float32_t& amplitude) {
        sampleRate = _sampleRate;
        frequency = _frequency;
        amplitude = _amplitude;
    }

    void render(AudioBufferFloat32& frameBuffer) {
        
        float32_t** samples = frameBuffer.getFrameData();
        float32_t yq;
        float32_t y;
        for (uint16_t i = 0; i < frameBuffer.getFrameCount(); ++i) {
            
            yq = _yq1 - (_epsilon * _y1);
            y = _y1 + (_epsilon * yq);
            
            // update delays
            _yq1 = yq;
            _y1 = y;
            
            for (uint16_t j = 0; j < frameBuffer.getChannelCount(); ++j) {
                samples[j][i] = _amplitude * y;
            }
        }
    }
};

#endif 

