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
    AudioGain() {
        initialize();
    }
    
    ~AudioGain() {
        finalize();
    }
    
    void initialize() {
        setParameters(1.0f,0.0f);
    }
    
    void finalize() {
    }
    
    void reset() {
        initialize();
    }
    
    void setParameters(const float gain, const float mute) {
        _gain = std::min(std::max(gain, 0.0f), 1.0f);
        _mute = mute != 0.0f;
        
    }
    
    void getParameters(float& gain, float& mute) {
        gain = _gain;
        mute = _mute ? 1.0f : 0.0f;
    }
    
    void render(AudioBufferFloat32& frameBuffer) {
        if (_mute) {
            frameBuffer.zeroFrames();
            return;
        } 
        
        float32_t** samples = frameBuffer.getFrameData();
        for (uint16_t j = 0; j < frameBuffer.getChannelCount(); ++j) {
            for (uint16_t i = 0; i < frameBuffer.getFrameCount(); i += 32) {
                samples[j][i + 0] *= _gain;
                samples[j][i + 1] *= _gain;
                samples[j][i + 2] *= _gain;
                samples[j][i + 3] *= _gain;
                samples[j][i + 4] *= _gain;
                samples[j][i + 5] *= _gain;
                samples[j][i + 6] *= _gain;
                samples[j][i + 7] *= _gain;
                samples[j][i + 8] *= _gain;
                samples[j][i + 9] *= _gain;
                samples[j][i + 10] *= _gain;
                samples[j][i + 11] *= _gain;
                samples[j][i + 12] *= _gain;
                samples[j][i + 13] *= _gain;
                samples[j][i + 14] *= _gain;
                samples[j][i + 15] *= _gain;
                samples[j][i + 16] *= _gain;
                samples[j][i + 17] *= _gain;
                samples[j][i + 18] *= _gain;
                samples[j][i + 19] *= _gain;
                samples[j][i + 20] *= _gain;
                samples[j][i + 21] *= _gain;
                samples[j][i + 22] *= _gain;
                samples[j][i + 23] *= _gain;
                samples[j][i + 24] *= _gain;
                samples[j][i + 25] *= _gain;
                samples[j][i + 26] *= _gain;
                samples[j][i + 27] *= _gain;
                samples[j][i + 28] *= _gain;
                samples[j][i + 29] *= _gain;
                samples[j][i + 30] *= _gain;
                samples[j][i + 31] *= _gain;
            }
        }
    }
};

#endif // AudioGain_h


