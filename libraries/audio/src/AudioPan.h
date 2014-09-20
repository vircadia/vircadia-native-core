//
//  AudioPan.h
//  hifi
//
//  Created by Craig Hansen-Sturm on 9/1/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AudioPan_h
#define hifi_AudioPan_h

class AudioPan
{
    float32_t _pan;
    float32_t _gainLeft;
    float32_t _gainRight;
  
    static float32_t ONE_MINUS_EPSILON;
    static float32_t ZERO_PLUS_EPSILON;
    static float32_t ONE_HALF_MINUS_EPSILON;
    static float32_t ONE_HALF_PLUS_EPSILON;
    
    void updateCoefficients();
    
public:
    AudioPan();
    ~AudioPan();
    
    void initialize();
    void finalize();
    void reset();
    
    void setParameters(const float32_t pan);
    void getParameters(float32_t& pan);
    
    void render(AudioBufferFloat32& frameBuffer);
};


inline void AudioPan::render(AudioBufferFloat32& frameBuffer) {
    
    if (frameBuffer.getChannelCount() != 2) {
        return;
    }
    
    float32_t** samples = frameBuffer.getFrameData();
    
    bool frameAlignment16 = (frameBuffer.getFrameCount() & 0x0F) == 0;
    if (frameAlignment16) {
        
        if (frameBuffer.getChannelCount() == 2) {
            
            for (uint32_t i = 0; i < frameBuffer.getFrameCount(); i += 16) {
                samples[0][i + 0] *= _gainLeft;
                samples[0][i + 1] *= _gainLeft;
                samples[0][i + 2] *= _gainLeft;
                samples[0][i + 3] *= _gainLeft;
                samples[0][i + 4] *= _gainLeft;
                samples[0][i + 5] *= _gainLeft;
                samples[0][i + 6] *= _gainLeft;
                samples[0][i + 7] *= _gainLeft;
                samples[0][i + 8] *= _gainLeft;
                samples[0][i + 9] *= _gainLeft;
                samples[0][i + 10] *= _gainLeft;
                samples[0][i + 11] *= _gainLeft;
                samples[0][i + 12] *= _gainLeft;
                samples[0][i + 13] *= _gainLeft;
                samples[0][i + 14] *= _gainLeft;
                samples[0][i + 15] *= _gainLeft;
                samples[1][i + 0] *= _gainRight;
                samples[1][i + 1] *= _gainRight;
                samples[1][i + 2] *= _gainRight;
                samples[1][i + 3] *= _gainRight;
                samples[1][i + 4] *= _gainRight;
                samples[1][i + 5] *= _gainRight;
                samples[1][i + 6] *= _gainRight;
                samples[1][i + 7] *= _gainRight;
                samples[1][i + 8] *= _gainRight;
                samples[1][i + 9] *= _gainRight;
                samples[1][i + 10] *= _gainRight;
                samples[1][i + 11] *= _gainRight;
                samples[1][i + 12] *= _gainRight;
                samples[1][i + 13] *= _gainRight;
                samples[1][i + 14] *= _gainRight;
                samples[1][i + 15] *= _gainRight;
            }
        }
        else {
            assert("unsupported channel format");
        }
    }
    else {
        for (uint32_t i = 0; i < frameBuffer.getFrameCount(); i += 1) {
            samples[0][i] *= _gainLeft;
            samples[1][i] *= _gainRight;
        }
    }
}

inline void AudioPan::updateCoefficients() {
    
    // implement constant power sin^2 + cos^2 = 1 panning law
    
    if (_pan >= ONE_MINUS_EPSILON) { // full right
        _gainLeft = 0.0f;
        _gainRight = 1.0f;
    }
    else if (_pan <= ZERO_PLUS_EPSILON) { // full left
        _gainLeft = 1.0f;
        _gainRight = 0.0f;
    }
    else if ((_pan >= ONE_HALF_MINUS_EPSILON) && (_pan <= ONE_HALF_PLUS_EPSILON)) {  // center
        _gainLeft = 1.0f / SQUARE_ROOT_OF_2;
        _gainRight = 1.0f / SQUARE_ROOT_OF_2;
    }
    else { // intermediate cases
        _gainLeft = cosf( TWO_PI * _pan );
        _gainRight = sinf( TWO_PI * _pan );
    }
}

#endif // AudioPan_h


