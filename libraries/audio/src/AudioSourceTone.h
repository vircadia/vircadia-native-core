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
    
    void updateCoefficients();
    
public:
    AudioSourceTone();
    ~AudioSourceTone();
    
    void initialize();
    void finalize();
    void reset();

    void setParameters(const float32_t sampleRate, const float32_t frequency,  const float32_t amplitude);    
    void getParameters(float32_t& sampleRate, float32_t& frequency, float32_t& amplitude);
    
    void render(AudioBufferFloat32& frameBuffer);
};


inline void AudioSourceTone::render(AudioBufferFloat32& frameBuffer) {
    float32_t** samples = frameBuffer.getFrameData();
    float32_t yq;
    float32_t y;
    for (uint32_t i = 0; i < frameBuffer.getFrameCount(); ++i) {
        
        yq = _yq1 - (_epsilon * _y1);
        y = _y1 + (_epsilon * yq);
        
        // update delays
        _yq1 = yq;
        _y1 = y;
        
        for (uint32_t j = 0; j < frameBuffer.getChannelCount(); ++j) {
            samples[j][i] = _amplitude * y;
        }
    }
}

#endif 

