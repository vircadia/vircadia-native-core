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

class AudioSourceTone
{
    static uint32_t _frameOffset;
    float32_t _frequency;
    float32_t _sampleRate;
    float32_t _omega;
    
public:
    AudioSourceTone() {
        initialize();
    }
    
    ~AudioSourceTone() {
        finalize();
    }
    
    void initialize() {
        _frameOffset = 0;
        setParameters(SAMPLE_RATE, 220.0f);
    }
    
    void finalize() {
    }
    
    void reset() {
        _frameOffset = 0;
    }

    void setParameters(const float32_t sampleRate, const float32_t frequency) {
        _sampleRate = std::max(sampleRate, 1.0f);
        _frequency = std::max(frequency, 1.0f);
        _omega = _frequency / _sampleRate * TWO_PI;
    }
    
    void getParameters(float32_t& sampleRate, float32_t& frequency) {
        sampleRate = _sampleRate;
        frequency = _frequency;
    }

    void render(AudioBufferFloat32& frameBuffer) {
        
        // note: this is a placeholder implementation. final version will not include any transcendental ops in our render loop
        
        float32_t** samples = frameBuffer.getFrameData();
        for (uint16_t i = 0; i < frameBuffer.getFrameCount(); ++i) {
            for (uint16_t j = 0; j < frameBuffer.getChannelCount(); ++j) {
                samples[j][i] = sinf((i + _frameOffset) * _omega);
            }
        }
        _frameOffset += frameBuffer.getFrameCount();
    }
};

#endif 

