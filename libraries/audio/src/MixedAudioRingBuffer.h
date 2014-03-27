//
//  MixedAudioRingBuffer.h
//  hifi
//
//  Created by Stephen Birarda on 2014-03-26.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__MixedAudioRingBuffer__
#define __hifi__MixedAudioRingBuffer__

#include "AudioRingBuffer.h"

class MixedAudioRingBuffer : public AudioRingBuffer {
    Q_OBJECT
public:
    MixedAudioRingBuffer(int numFrameSamples);
    
    float getLastReadFrameAverageLoudness() const { return _lastReadFrameAverageLoudness; }
    
    qint64 readSamples(int16_t* destination, qint64 maxSamples);    
private:
     float _lastReadFrameAverageLoudness;
};

#endif /* defined(__hifi__MixedAudioRingBuffer__) */
