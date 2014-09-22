//
//  AudioGain.h
//  hifi
//
//  Created by Craig Hansen-Sturm on 9/1/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AudioGain_h
#define hifi_AudioGain_h

class AudioGain
{
    float32_t _gain;
    bool _mute;
    
public:
    AudioGain();
    ~AudioGain();
    
    void initialize();
    void finalize();
    void reset();
    
    void setParameters(const float gain, const float mute);
    void getParameters(float& gain, float& mute);
    
    void render(AudioBufferFloat32& frameBuffer);
};


inline void AudioGain::render(AudioBufferFloat32& frameBuffer) {
    if (_mute) {
        frameBuffer.zeroFrames();
        return;
    } 
    
    float32_t** samples = frameBuffer.getFrameData();
    
    bool frameAlignment16 = (frameBuffer.getFrameCount() & 0x0F) == 0;
    if (frameAlignment16) {
        
        if (frameBuffer.getChannelCount() == 1) {
            
            for (uint32_t i = 0; i < frameBuffer.getFrameCount(); i += 16) {
                samples[0][i + 0] *= _gain;
                samples[0][i + 1] *= _gain;
                samples[0][i + 2] *= _gain;
                samples[0][i + 3] *= _gain;
                samples[0][i + 4] *= _gain;
                samples[0][i + 5] *= _gain;
                samples[0][i + 6] *= _gain;
                samples[0][i + 7] *= _gain;
                samples[0][i + 8] *= _gain;
                samples[0][i + 9] *= _gain;
                samples[0][i + 10] *= _gain;
                samples[0][i + 11] *= _gain;
                samples[0][i + 12] *= _gain;
                samples[0][i + 13] *= _gain;
                samples[0][i + 14] *= _gain;
                samples[0][i + 15] *= _gain;
            }
        } else if (frameBuffer.getChannelCount() == 2) {
            
            for (uint32_t i = 0; i < frameBuffer.getFrameCount(); i += 16) {
                samples[0][i + 0] *= _gain;
                samples[0][i + 1] *= _gain;
                samples[0][i + 2] *= _gain;
                samples[0][i + 3] *= _gain;
                samples[0][i + 4] *= _gain;
                samples[0][i + 5] *= _gain;
                samples[0][i + 6] *= _gain;
                samples[0][i + 7] *= _gain;
                samples[0][i + 8] *= _gain;
                samples[0][i + 9] *= _gain;
                samples[0][i + 10] *= _gain;
                samples[0][i + 11] *= _gain;
                samples[0][i + 12] *= _gain;
                samples[0][i + 13] *= _gain;
                samples[0][i + 14] *= _gain;
                samples[0][i + 15] *= _gain;
                samples[1][i + 0] *= _gain;
                samples[1][i + 1] *= _gain;
                samples[1][i + 2] *= _gain;
                samples[1][i + 3] *= _gain;
                samples[1][i + 4] *= _gain;
                samples[1][i + 5] *= _gain;
                samples[1][i + 6] *= _gain;
                samples[1][i + 7] *= _gain;
                samples[1][i + 8] *= _gain;
                samples[1][i + 9] *= _gain;
                samples[1][i + 10] *= _gain;
                samples[1][i + 11] *= _gain;
                samples[1][i + 12] *= _gain;
                samples[1][i + 13] *= _gain;
                samples[1][i + 14] *= _gain;
                samples[1][i + 15] *= _gain;
            }
        } else {
            assert("unsupported channel format");
        }
    } else {
        
        for (uint32_t j = 0; j < frameBuffer.getChannelCount(); ++j) {
            for (uint32_t i = 0; i < frameBuffer.getFrameCount(); i += 1) {
                samples[j][i] *= _gain;
            }
        }
    }
}

#endif // AudioGain_h


