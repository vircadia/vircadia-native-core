//
//  AudioNoiseGate.h
//  libraries/audio
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

const int NUMBER_OF_NOISE_SAMPLE_BLOCKS = 300;

class AudioNoiseGate {
public:
    AudioNoiseGate();

    void gateSamples(int16_t* samples, int numSamples);
    void removeDCOffset(int16_t* samples, int numSamples);

    bool clippedInLastBlock() const { return _didClipInLastBlock; }
    bool closedInLastBlock() const { return _closedInLastBlock; }
    bool openedInLastBlock() const { return _openedInLastBlock; }
    bool isOpen() const { return _isOpen; }
    float getMeasuredFloor() const { return _measuredFloor; }
    float getLastLoudness() const { return _lastLoudness; }

    static const float CLIPPING_THRESHOLD;

private:
    float _lastLoudness;
    bool _didClipInLastBlock;
    float _dcOffset;
    float _measuredFloor;
    float _sampleBlocks[NUMBER_OF_NOISE_SAMPLE_BLOCKS];
    int _sampleCounter;
    bool _isOpen;
    bool _closedInLastBlock { false };
    bool _openedInLastBlock { false };
    int _blocksToClose;
};

#endif // hifi_AudioNoiseGate_h
