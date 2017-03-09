//
//  AudioNoiseGate.h
//  interface/src/audio
//
//  Created by Stephen Birarda on 2014-12-16.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AudioNoiseGate_h
#define hifi_AudioNoiseGate_h

#include <stdint.h>

const int NUMBER_OF_NOISE_SAMPLE_FRAMES = 300;

class AudioNoiseGate {
public:
    AudioNoiseGate();
    
    void gateSamples(int16_t* samples, int numSamples);
    void removeDCOffset(int16_t* samples, int numSamples);
    
    bool clippedInLastFrame() const { return _didClipInLastFrame; }
    bool closedInLastFrame() const { return _closedInLastFrame; }
    float getMeasuredFloor() const { return _measuredFloor; }
    float getLastLoudness() const { return _lastLoudness; }
    
    static const float CLIPPING_THRESHOLD;
    
private:
    int _inputFrameCounter;
    float _lastLoudness;
    float _quietestFrame;
    float _loudestFrame;
    bool _didClipInLastFrame;
    float _dcOffset;
    float _measuredFloor;
    float _sampleFrames[NUMBER_OF_NOISE_SAMPLE_FRAMES];
    int _sampleCounter;
    bool _isOpen;
    bool _closedInLastFrame { false };
    int _framesToClose;
};

#endif // hifi_AudioNoiseGate_h