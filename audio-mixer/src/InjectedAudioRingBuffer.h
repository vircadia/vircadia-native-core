//
//  InjectedAudioRingBuffer.h
//  hifi
//
//  Created by Stephen Birarda on 6/5/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__InjectedAudioRingBuffer__
#define __hifi__InjectedAudioRingBuffer__

#include <AudioInjector.h>

#include "PositionalAudioRingBuffer.h"

class InjectedAudioRingBuffer : public PositionalAudioRingBuffer {
public:
    InjectedAudioRingBuffer();
    
    float getRadius() const { return _radius; }
    float getAttenuationRatio() const { return _attenuationRatio; }
    const unsigned char* getStreamIdentifier() const { return _streamIdentifier; }
private:
    // disallow copying of InjectedAudioRingBuffer objects
    InjectedAudioRingBuffer(const InjectedAudioRingBuffer&);
    InjectedAudioRingBuffer& operator= (const InjectedAudioRingBuffer&);
    
    float _radius;
    float _attenuationRatio;
    unsigned char _streamIdentifier[STREAM_IDENTIFIER_NUM_BYTES];
};

#endif /* defined(__hifi__InjectedAudioRingBuffer__) */
