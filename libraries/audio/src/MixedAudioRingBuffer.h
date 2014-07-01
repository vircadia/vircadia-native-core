//
//  MixedAudioRingBuffer.h
//  libraries/audio/src
//
//  Created by Stephen Birarda on 2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_MixedAudioRingBuffer_h
#define hifi_MixedAudioRingBuffer_h

#include "AudioRingBuffer.h"

class MixedAudioRingBuffer : public AudioRingBuffer {
    Q_OBJECT
public:
    MixedAudioRingBuffer(int numFrameSamples, int numFramesCapacity);
    
    float getLastReadFrameAverageLoudness() const { return _lastReadFrameAverageLoudness; }
    
    qint64 readSamples(int16_t* destination, qint64 maxSamples);    
private:
     float _lastReadFrameAverageLoudness;
};

#endif // hifi_MixedAudioRingBuffer_h
